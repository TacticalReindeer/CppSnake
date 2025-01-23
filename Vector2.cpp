#include <ostream>
#include "Vector2.h"

// definition of class Vector2 and Vector2Int

#pragma region Vector2

Vector2::Vector2() {}

std::ostream& operator<<(std::ostream& os, Vector2& vector)
{
    return os << "( " << vector.x << ", " << vector.y << ")";
}

std::ostream& operator<<(std::ostream& os, Vector2&& vector)
{
    return os << "( " << vector.x << ", " << vector.y << ")";
}

#pragma endregion

#pragma region Vector2Int

Vector2Int::Vector2Int() {}

constexpr Vector2Int Vector2Int::up(0, 1);
constexpr Vector2Int Vector2Int::down(0, -1);
constexpr Vector2Int Vector2Int::left(-1, 0);
constexpr Vector2Int Vector2Int::right(1, 0);

Vector2Int& Vector2Int::operator+=(Vector2Int rVal)
{
    x += rVal.x;
    y += rVal.y;
    return *this;
}

Vector2Int Vector2Int::operator+(Vector2Int rVal)
{
    Vector2Int temp = *this;
    return temp += rVal;
}

Vector2Int& Vector2Int::operator-=(Vector2Int rVal)
{
    x -= rVal.x;
    y -= rVal.y;
    return *this;
}

Vector2Int Vector2Int::operator-(Vector2Int rVal)
{
    Vector2Int temp = *this;
    return temp += rVal;
}

std::ostream& operator<<(std::ostream& os, Vector2Int& vector)
{
    return os << "( " << vector.x << ", " << vector.y << ")";
}

std::ostream& operator<<(std::ostream& os, Vector2Int&& vector)
{
    return os << "( " << vector.x << ", " << vector.y << ")";
}

#pragma endregion