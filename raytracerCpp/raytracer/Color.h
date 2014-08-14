
#ifndef _Color_h_
#define _Color_h_

struct Color{
	float r;
	float g;
	float b;
	inline Color(float _r,float _g,float _b):r(_r),g(_g),b(_b) {  };

    inline Color add(const Color& c)const { return Color(r + c.r, g + c.g, b + c.b); }
    inline Color multiply(float s) const { return Color(r * s, g * s, b * s); }
    inline Color modulate(const Color& c) const { return Color(r * c.r, g * c.g, b * c.b); }
	inline void saturate() { r = std::min(r, (float)1); g = std::min(g, (float)1); b = std::min(b, (float)1); }

	static inline Color black(){ return Color(0,0,0); }
	static inline Color white(){ return Color(1,1,1); }
	static inline Color red()  { return Color(1,0,0); }
	static inline Color green(){ return Color(0,1,0); }
	static inline Color blue() { return Color(0,0,1); }
};

#endif