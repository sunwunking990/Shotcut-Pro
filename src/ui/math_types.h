#pragma once

#include <cmath>

namespace Shotcut::UI {

// Simple vector2 class to replace GLM vec2
struct vec2 {
    float x, y;
    
    vec2() : x(0.0f), y(0.0f) {}
    vec2(float x, float y) : x(x), y(y) {}
    vec2(float v) : x(v), y(v) {}
    
    vec2 operator+(const vec2& other) const { return vec2(x + other.x, y + other.y); }
    vec2 operator-(const vec2& other) const { return vec2(x - other.x, y - other.y); }
    vec2 operator*(float s) const { return vec2(x * s, y * s); }
    vec2 operator/(float s) const { return vec2(x / s, y / s); }
    
    vec2& operator+=(const vec2& other) { x += other.x; y += other.y; return *this; }
    vec2& operator-=(const vec2& other) { x -= other.x; y -= other.y; return *this; }
    vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    vec2& operator/=(float s) { x /= s; y /= s; return *this; }
    
    float length() const { return std::sqrt(x * x + y * y); }
    float dot(const vec2& other) const { return x * other.x + y * other.y; }
};

// Simple vector4 class for colors
struct vec4 {
    float x, y, z, w;
    
    vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(float v) : x(v), y(v), z(v), w(v) {}
};

// Type aliases for compatibility
namespace glm {
    using vec2 = Shotcut::UI::vec2;
    using vec4 = Shotcut::UI::vec4;
}

} // namespace Shotcut::UI
