#ifndef POINT_H
#define POINT_H


class Point
{
public:
    Point();
    Point(double x, double y);
    double getX();
    double getY();
    void setX(double x);
    void setY(double y);

private:
    double x, y;
};

#endif // POINT_H
