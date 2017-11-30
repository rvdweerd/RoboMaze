#pragma once

#include "Mouse.h"
#include "Graphics.h"
#include "TileMap.h"
#include <vector>
#include <memory>
#include "Robo.h"
#include "CamController.h"

namespace Window
{
	class Window
	{
	public:
		Window( const RectI& region )
			:
			region( region )
		{}
		virtual void Update( float dt ) {}
		virtual Window* OnMouseEvent( const Mouse::Event& e )
		{
			isHover = true;
			return this;
		}
		virtual void OnMouseLeave()
		{
			isHover = false;
		}
		virtual void Draw( Graphics& gfx ) const = 0;
		virtual ~Window() = default;
		bool IsHit( const Mouse::Event& e ) const
		{
			return region.Contains( e.GetPos() );
		}
		const RectI& GetRegion() const
		{
			return region;
		}
		bool IsHover() const
		{
			return isHover;
		}
	private:
		bool isHover = false;
		RectI region;
	};

	class Container : public Window
	{
	public:
		using Window::Window;
		void AddWindow( std::unique_ptr<Window> pWindow )
		{
			windowPtrs.push_back( std::move( pWindow ) );
		}
		virtual void Update( float dt )
		{
			for( const auto& p : windowPtrs )
			{
				p->Update( dt );
			}
		}
		virtual Window* OnMouseEvent( const Mouse::Event& e )
		{
			for( auto& p : windowPtrs )
			{
				if( p->IsHit( e ) )
				{
					return p->OnMouseEvent( e );
				}
			}
			return nullptr;
		}
		virtual void Draw( Graphics& gfx ) const override
		{
			for( const auto& p : windowPtrs )
			{
				p->Draw( gfx );
			}
		}
	private:
		std::vector<std::unique_ptr<Window>> windowPtrs;
	};

	class TextBox : public Window
	{
	public:
		TextBox( const RectI& rect,const Font& font )
			:
			Window( rect ),
			font( font )
		{}
		void SetText( const std::string& str )
		{
			text = str;
		}
		void SetTextColor( Color c )
		{
			color = c;
		}
		const std::string& GetText() const
		{
			return text;
		}
		void Draw( Graphics& gfx ) const override
		{
			gfx.DrawRect( GetRegion(),bgColor,Graphics::GetScreenRect() );
			font.DrawText( text,font.CalculateTextRect( text ).TopLeft() + GetRegion().GetCenter(),color,gfx );
		}
	private:
		const Font& font;
		std::string text;
		Color color = Colors::White;
	protected:
		Color bgColor = { 255,28,28,28 };
	};

	class CamLockToggle : public TextBox
	{
	public:
		CamLockToggle( const RectI& rect,const Font& font )
			:
			TextBox( rect,font ),
			bgColorLo( bgColor )
		{
			Update();
		}
		void Draw( Graphics& gfx ) const override
		{
			if( IsHover() )
			{
				const_cast<CamLockToggle*>(this)->bgColor = bgColorHi; // oh my
			}
			TextBox::Draw( gfx );
			const_cast<CamLockToggle*>(this)->bgColor = bgColorLo;
		}
		Window* OnMouseEvent( const Mouse::Event& e ) override
		{
			if( e.GetType() == Mouse::Event::Type::LPress )
			{
				Toggle();
			}
			return Window::OnMouseEvent( e );;
		}
		void Toggle()
		{
			locked = !locked;
			Update();
		}
		bool IsLocked() const
		{
			return locked;
		}
	private:
		void Update()
		{
			if( locked )
			{
				SetText( "LCK" );
				SetTextColor( Colors::Red );
			}
			else
			{
				SetText( "MAN" );
				SetTextColor( Colors::White );
			}
		}
	private:
		Color bgColorHi = { 255,100,100,150 };
		Color bgColorLo;
		bool locked = true;
	};

	class Map : public Window
	{
	public:
		Map( RectI& region,TileMap& map,const Robo& rob,const CamLockToggle& lockToggle )
			:
			Window( region ),
			map( map ),
			rob( rob ),
			port( region ),
			cam( port,map.GetMapBounds() ),
			scroll( cam ),
			cc( cam,map,rob ),
			lockToggle( lockToggle )
		{}
		Window* OnMouseEvent( const Mouse::Event& e ) override
		{
			if( !lockToggle.IsLocked() )
			{
				switch( e.GetType() )
				{
				case Mouse::Event::Type::Move:
					if( port.GetClipRect().Contains( e.GetPos() ) )
					{
						scroll.MoveTo( e.GetPos() );
					}
					else
					{
						scroll.Release();
					}
					break;
				case Mouse::Event::Type::LPress:
					if( port.GetClipRect().Contains( e.GetPos() ) )
					{
						scroll.Grab( e.GetPos() );
					}
					break;
				case Mouse::Event::Type::LRelease:
					scroll.Release();
					break;
				}
			}
			return Window::OnMouseEvent( e );
		}
		void Draw( Graphics& gfx ) const override
		{
			map.Draw( gfx,cam,port );
			rob.Draw( gfx,cam,port,map );
		}
		void OnMouseLeave() override
		{
			Window::OnMouseLeave();
			scroll.Release();
		}
		void Update( float dt ) override
		{
			if( lockToggle.IsLocked() )
			{
				cc.Update( dt );
			}
		}

	private:
		TileMap& map;
		const Robo& rob;
		Viewport port;
		Camera cam;
		CamController cc;
		ScrollRegion scroll;
		const CamLockToggle& lockToggle;
	};
	
	class SimstepControllable
	{
	public:
		void SetTickDuration( float td )
		{
			tick_duration = td;
		}
		bool IsPaused() const
		{
			return isPaused;
		}
		void Pause()
		{
			isPaused = true;
			accumulated_time = 0.0f;
		}
		void Unpause()
		{
			isPaused = false;
		}
		void SingleStep()
		{
			OnTick();
		}
		float TimeRemaining() const
		{
			return tick_duration - accumulated_time;
		}
		bool Update( float dt )
		{
			bool updateOccurred = false;
			if( !isPaused )
			{
				// process game tick
				accumulated_time += dt;
				// consume time in chunks of tick_duration
				while( accumulated_time >= tick_duration )
				{
					accumulated_time -= tick_duration;
					OnTick();
					updateOccurred = true;
				}
			}
			return updateOccurred;
		}
		virtual void OnTick() = 0;
	protected:
		bool isPaused = true;
		float accumulated_time = 0.0f;
		float tick_duration = 0.15f;
	};

	class PlayPause : public Window
	{
	public:
		PlayPause( const Vei2& pos,int size,int margin,SimstepControllable& sim )
			:
			Window( RectI{ pos,size,size } ),
			margin( margin ),
			sim( sim )
		{}
		Window* OnMouseEvent( const Mouse::Event& e ) override
		{
			Window::OnMouseEvent( e );
			if( e.GetType() == Mouse::Event::Type::LPress )
			{
				if( sim.IsPaused() )
				{
					sim.Unpause();
				}
				else
				{
					sim.Pause();
				}
			}
			return this;
		}
		void Draw( Graphics& gfx ) const override
		{
			if( IsHover() )
			{
				gfx.DrawRect( GetRegion(),{ 255,100,100,150 },Graphics::GetScreenRect() );
			}
			if( sim.IsPaused() )
			{
				const auto play_width = ContentWidth() / 2;
				const Vei2 play_tl = GetRegion().GetCenter() - Vei2{ play_width / 2,play_width };
				gfx.DrawIsoRightTriBL( play_tl.x,play_tl.y,play_width,Colors::White );
				gfx.DrawIsoRightTriUL( play_tl.x,play_tl.y + play_width,play_width,Colors::White );
			}
			else
			{
				RectI bar = {
					{ GetRegion().GetCenter().x - ContentWidth() / 4 - BarWidth() / 2,GetRegion().top + margin },
					BarWidth(),
					ContentWidth()
				};
				gfx.DrawRect( bar,{ Colors::White,255 },Graphics::GetScreenRect() );
				bar.DisplaceBy( { ContentWidth() / 2,0 } );
				gfx.DrawRect( bar,{ Colors::White,255 },Graphics::GetScreenRect() );
			}
		}
	private:
		int ContentWidth() const
		{
			return GetRegion().GetWidth() - margin * 2;
		}
		int BarWidth() const
		{
			return ContentWidth() / 3;
		}
	private:
		int margin;
		SimstepControllable& sim;
	};

	class SingleStep : public Window
	{
	public:
		SingleStep( const Vei2& pos,int size,int margin,SimstepControllable& sim )
			:
			Window( RectI{ pos,size,size } ),
			margin( margin ),
			sim( sim )
		{}
		Window* OnMouseEvent( const Mouse::Event& e ) override
		{
			Window::OnMouseEvent( e );
			if( e.GetType() == Mouse::Event::Type::LPress )
			{
				if( sim.IsPaused() )
				{
					sim.SingleStep();
				}
			}
			return this;
		}
		void Draw( Graphics& gfx ) const override
		{
			if( sim.IsPaused() )
			{
				if( IsHover() )
				{
					gfx.DrawRect( GetRegion(),{ 255,100,100,150 },Graphics::GetScreenRect() );
				}
				const Vei2 bar_tl = GetRegion().TopLeft() + Vei2{ margin,margin };
				gfx.DrawRect( { bar_tl,BarWidth(),ContentWidth() },{ Colors::White,255 },Graphics::GetScreenRect() );

				const auto play_width = ContentWidth() / 2;
				const Vei2 play_tl = { GetRegion().GetCenter().x,bar_tl.y };
				gfx.DrawIsoRightTriBL( play_tl.x,play_tl.y,play_width,Colors::White );
				gfx.DrawIsoRightTriUL( play_tl.x,play_tl.y + play_width,play_width,Colors::White );
			}
		}
	private:
		int ContentWidth() const
		{
			return GetRegion().GetWidth() - margin * 2;
		}
		int BarWidth() const
		{
			return ContentWidth() / 3;
		}
	private:
		int margin;
		SimstepControllable& sim;
	};

	class SpeedSlider : public Window
	{
	public:
		SpeedSlider( const RectI& rect,int slider_width,int margin,float min,float max,float percent,SimstepControllable& sim )
			:
			Window( rect ),
			margin( margin ),
			slider_width( slider_width ),
			sim( sim ),
			min( min ),
			k( max - min )
		{
			sim.SetTickDuration( CalculateValue() );
			SetSliderPercent( percent );
		}
		Window* OnMouseEvent( const Mouse::Event& e ) override
		{
			switch( e.GetType() )
			{
			case Mouse::Event::Type::Move:
				if( isGrabbing )
				{
					slider_offset += e.GetPosX() - grab_base_x;
					slider_offset = std::min( slider_offset,MaxSliderOffset() );
					slider_offset = std::max( slider_offset,0 );
					sim.SetTickDuration( CalculateValue() );
					grab_base_x = e.GetPosX();
				}
				if( GetHandleRect().Contains( e.GetPos() ) )
				{
					handle_hover = true;
				}
				else
				{
					handle_hover = false;
				}
				break;
			case Mouse::Event::Type::LPress:
				if( GetHandleRect().Contains( e.GetPos() ) )
				{
					isGrabbing = true;
					grab_base_x = e.GetPosX();
				}
				break;
			case Mouse::Event::Type::LRelease:
				isGrabbing = false;
				break;
			}
			return this;
		}
		void Draw( Graphics& gfx ) const override
		{
			const auto cr = ContentRect();
			const RectI track_rect = {
				{ cr.left,cr.GetCenter().y - GetTrackHeight() / 2 },
				cr.GetWidth(),
				GetTrackHeight()
			};
			gfx.DrawRect( track_rect,{ Colors::Gray,255 },Graphics::GetScreenRect() );
			const Color hc = handle_hover ? Color{ 255,170,170,255 } : Color{ Colors::White,255 };
			gfx.DrawRect( GetHandleRect(),hc,Graphics::GetScreenRect() );
		}
		void OnMouseLeave() override
		{
			Window::OnMouseLeave();
			isGrabbing = false;
			handle_hover = false;
		}
		void SetSliderPercent( float p )
		{
			// clamp percent
			p = std::min( std::max( p,0.0f ),1.0f );
			slider_offset = int( float( MaxSliderOffset() ) * p );
		}
	private:
		float CalculateValue() const
		{
			return k * std::pow( 1.0f - GetSliderPercent(),q ) + min;
		}
		RectI ContentRect() const
		{
			return GetRegion().GetExpanded( -margin );
		}
		int MaxSliderOffset() const
		{
			return ContentRect().GetWidth() - slider_width;
		}
		float GetSliderPercent() const
		{
			return float(slider_offset) / float(MaxSliderOffset());
		}
		int GetTrackHeight() const
		{
			return ContentRect().GetHeight() / 4;
		}
		RectI GetHandleRect() const
		{
			auto cr = ContentRect();
			cr.left += slider_offset;
			cr.right = cr.left + slider_width;
			return cr;
		}
	private:
		bool handle_hover = false;
		int margin;
		int slider_width;
		int slider_offset = 0;
		int grab_base_x;
		bool isGrabbing = false;
		float min;
		// this is the order of the polynomial curve
		float q = 5.0f;
		// this is coef is calculated to set 0->min 1->max
		float k;
		SimstepControllable& sim;
	};
}

// TODO: default hardwired Window behavior with calling protected virtual
// TODO: buttons inherit from common