#ifndef VECTOR2_H
#define VECTOR2_H

#include <ostream>

struct Vector2
{
    public:
    float x, y;

    Vector2();
    constexpr Vector2(float x, float y) : x(x), y(y) {}

    friend std::ostream& operator<<(std::ostream& os, Vector2& vector);
    friend std::ostream& operator<<(std::ostream& os, Vector2&& vector);
};

struct Vector2Int
{
    public:
    int x, y;

    Vector2Int();
    constexpr Vector2Int(int x, int y) : x(x), y(y) {}

    static const Vector2Int up;
    static const Vector2Int down;
    static const Vector2Int left;
    static const Vector2Int right;

    friend std::ostream& operator<<(std::ostream& os, Vector2Int& vector);
    friend std::ostream& operator<<(std::ostream& os, Vector2Int&& vector);

    Vector2Int& operator+=(Vector2Int rVal);
    Vector2Int operator+(Vector2Int rVal);
    Vector2Int& operator-=(Vector2Int rVal);
    Vector2Int operator-(Vector2Int rVal);

    operator std::string() const
    {
        return "( " + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
};

#endif