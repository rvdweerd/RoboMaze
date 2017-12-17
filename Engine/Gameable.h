#pragma once

class Gameable
{
public:
	virtual void Draw( class Graphics& gfx ) const = 0;
	virtual void Update( class MainWindow& wnd,float dt ) = 0;
	virtual ~Gameable() = default;
};