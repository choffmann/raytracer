#include <iostream>
#include <math.h>
#include <fstream>
#include <vector>
#include <memory>
#include <limits>


// url: https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-ray-tracing

using namespace std;

/** 3D vector in cartesian space.
 *
 */
struct Vec3d {
    double x, y, z;
    Vec3d() : x{0}, y{0}, z{0} {}
    Vec3d(double x_, double y_, double z_) : x{x_}, y{y_}, z{z_} {}
    Vec3d(const Vec3d& v) : x{v.x}, y{v.y}, z{v.z} {}

    Vec3d& operator-= (const Vec3d& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    Vec3d& operator/= (const double v)
    {
        x /= v;
        y /= v;
        z /= v;
        return *this;
    }

    Vec3d& operator+= (const Vec3d& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vec3d& operator*= (double val)
    {
        x *= val;
        y *= val;
        z *= val;
        return *this;
    }

    double dotProduct(const Vec3d& vec2) const
    {
        return (x * vec2.x) +
               (y * vec2.y) +
               (z * vec2.z);
    }

    double length() const
    {
        return sqrt(dotProduct(*this));
    }

    Vec3d& normalize()
    {
        double l = length();
        *this /= l;
        return *this;
    }
};


Vec3d operator- (Vec3d lhs, const Vec3d& rhs)
{
    return lhs -= rhs;
}

Vec3d operator/ (Vec3d lhs, const double v)  {
    return lhs /= v;
}

Vec3d operator+ (Vec3d lhs, const Vec3d& rhs) {
    return lhs += rhs;
}

Vec3d operator* (Vec3d lhs, double val) {
    return lhs *= val;
}

/** Line which will be used for back raytracing
 *
 */
struct Ray {
    // origin
    Vec3d origin;
    // direction
    Vec3d direction;

    Ray(Vec3d o, Vec3d d): origin{o}, direction{d} {}

    Vec3d getPoint(double dist) const
    {
        return origin + direction*dist;
    }
};


/** Generic base class for all objects which shall be rendered
 *
 */
struct Object {
    virtual bool intersect(const Ray& ray, double& dist) const = 0;
    virtual Vec3d getNormal(const Vec3d& vec) const = 0;

};

/** The most common object to be rendered in a raytracer
 *
 */
struct Sphere : public Object {
    // define location + radius
    Vec3d center;
    double radius;

    Sphere(Vec3d c, double r): center{c}, radius{r} {}

    // get intersection with ray: refer: https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
    // more details: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
    virtual bool
    intersect(const Ray& ray, double& dist) const override
    {
        constexpr double eps = 0.00001;
        // (l * (o - c))^2 - || o - c ||^2 + r^2
        double val1, val2, val3, dist1, dist2;
        const Vec3d temp = ray.origin-center;
        val1 = temp.dotProduct(ray.direction);
        val2 = temp.length();
        val3 = val1*val1 - val2*val2 + radius*radius;

        if(val3 < 0) {
            return false;
        }

        // compute distance
        dist1 = -temp.dotProduct(ray.direction) + sqrt(val3);
        dist2 = -temp.dotProduct(ray.direction) - sqrt(val3);

        if(dist1 < 0 || dist2 < 0) {
            dist = max(dist1,dist2);
        } else if (dist1 > 0 && dist2 > 0) {
            dist = min(dist1,dist2);
        }

        return dist > eps;      //  neg. dist are behind ray; eps is for not hitting itself
    }

    virtual Vec3d
    getNormal(const Vec3d& P) const override
    {
        // src: https://cs.colorado.edu/~mcbryan/5229.03/mail/110.htm
        Vec3d n{P - center};
        n = n / radius;
        return n;
    }
};


/** Clips the value to min and max.
 * If the input value lies in [min,max], it will not be changed. Otherwise it will be set to min if val < min or to
 * max if val > max.
 * @param min Min value
 * @param max Max value
 * @param val Input value
 * @return The clipped value
 */
template <typename T>
T clamp(T min, T max, T val) {
    val = val < min ? min : val;
    val = val > max ? max : val;
    return val;
}

/** Container to save pixel color information in rgb format.
 *
 */
struct Color {
    double r,g,b;
    Color(): r{0}, g{0}, b{0} {};
    Color(double r_, double g_, double b_): r{r_}, g{g_}, b{b_} {};

    Color& operator*=(double d) {
        r *= d;
        g *= d;
        b *= d;
        return *this;
    }

    Color& operator*=(Color c) {
        r *= c.r;
        g *= c.g;
        b *= c.b;
        return *this;
    }

    Color operator+= (const Color& rhs) {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }

    Color& mult(double d) {
        r *= d;
        g *= d;
        b *= d;

        return *this;
    }

    Color& clamp(double min, double max) {
        r = ::clamp(min, max, r);
        g = ::clamp(min, max, g);
        b = ::clamp(min, max, b);
        return *this;
    }

    Color& round() {
        r = ::round(r);
        g = ::round(g);
        b = ::round(b);
        return *this;
    }

    static Color white() {
        return Color(1,1,1);
    }
    static Color black() {
        return Color();
    }
    static Color red() {
        return Color(1,0,0);
    }
 };

Color operator+(Color lhs, const Color& rhs) {
    return lhs += rhs;
}

Color operator*(Color lhs, const double d) {
    lhs *= d;
    return lhs;
}

Color operator*(Color lhs, const Color& rhs) {
    lhs *= rhs;
    return lhs;
}


struct Light {
    Vec3d pos;
    Color color;

    Light(Vec3d pos_, Color color_): pos{pos_}, color{color_} {}
    Light(Vec3d pos_): pos{pos_}, color{Color::white()} {}
};



ostream&
operator<<(ostream& os, const Color& c) {
    os << c.r << " " << c.g << " " << c.b << " ";
    return os;
}

ostream&
operator<<(ostream& os, const Vec3d& v) {
    os << v.x << " " << v.y << " " << v.z << " ";
    return os;
}


/** Converts angle in degree into rad.
 *
 * @param ang angle in degree
 * @return angle in rad
 */
double deg2rad(double ang) {
    return ang * M_PI / 180;
}


void check_op_overloading() {
    Vec3d v1, v2{1,2,3};

    v1 += v2;
    cout << v1 << " == 1 2 3\n";
    cout << v1*2 << " == 2 4 6\n";
//    cout << v1+2 << " == 3 4 5\n";

    Color c1, c2{1,2,3};
    c1 += c2;
    cout << c1 << " == 1 2 3\n";
    cout << c1*c2 << " == 1 4 9\n";
    cout << c1*2 << " == 2 4 6\n";
}


int
main(int argc, char** argv)
{
    cout << "... start ray tracer" << endl;
    check_op_overloading();

    constexpr unsigned int H = 500;
    constexpr unsigned int W = 800;
    constexpr unsigned int MAX_VAL = 255;
    constexpr double ASPECT_RATIO = (double)W/H;
    constexpr double FOV = 100;

    ofstream ofs{"out4.ppm"};    // http://netpbm.sourceforge.net/doc/ppm.html
    ofs << "P3\n"
        << to_string(W) << " " << to_string(H) << "\n"
        << to_string(MAX_VAL) << "\n";

    vector<shared_ptr<Object>> objects;
    objects.push_back(make_shared<Sphere>(Vec3d{0,0,-20}, 5));
//    objects.push_back(make_shared<Sphere>(Vec{10,0,-20}, 5));
    objects.push_back(make_shared<Sphere>(Vec3d{2,1,-15}, 1));
    objects.push_back(make_shared<Sphere>(Vec3d{4,4,-22}, 2.5));
    objects.push_back(make_shared<Sphere>(Vec3d{80,-6,-150}, 5));
    objects.push_back(make_shared<Sphere>(Vec3d{-4,4,-5}, 2.5));

//    s.radius = 10;
    Color background{0,0.5,0.5};
    Color scolor = Color::red();

    Color* img = new Color[W*H];
    double* zbuff = new double[W*H];
    Color* img_ptr = img;
    const Vec3d origin{0,0,0};  // center of projection

    vector<Light> lights;
    lights.emplace_back(Light{Vec3d{30,30,-2}, Color::white()});
//    lights.emplace_back(Light{Vec{-30,-20,1}});

    img_ptr = img;
    for (unsigned int y = 0; y<H; ++y) {
        for (unsigned int x = 0; x<W; ++x) {

            const double px_ndc = (x+0.5)/W;
            const double py_ndc = (y+0.5)/H;
            const double cam_x = (2*px_ndc  - 1) * ASPECT_RATIO * tan(deg2rad(FOV)/2);
            const double cam_y = (1 - 2*py_ndc) * tan(deg2rad(FOV)/2);

            Vec3d d{cam_x, cam_y, -1};
            d.normalize();
            const Ray ray{origin, d};

            double dist{std::numeric_limits<double>::max()}, tmpdist;
            Color px;
            bool intersect{false};
            Object* curo{nullptr};


            // check intersections
            for (const auto& o : objects) {
                if ( o->intersect(ray, tmpdist) ) {
                    if (tmpdist < dist) {
                        dist = tmpdist;
                        curo = &(*o);
                    }
                }
            }

            if (curo != nullptr) {
                Vec3d pintersect{ray.getPoint(dist)};
                const Vec3d n{curo->getNormal(pintersect)};

                for (const auto& l : lights) {
                    Vec3d lv{l.pos - pintersect};
//                    Vec lv{l.pos};
                    lv.normalize();
                    bool inShadow{false};

                    // check if object is blocking light
                    Ray shadow_ray{pintersect, lv};
                    for (const auto& o : objects) {
                        if ( o->intersect(shadow_ray, tmpdist) ) {
                            inShadow = true;
                            break;
                        }
                    }
                    if (inShadow) {
                        continue;
                    }

                    const double diff_factor{n.dotProduct(lv)};
                    px += scolor * l.color * diff_factor;
                }


                px.clamp(0, 1);
                px = px + scolor * 0.1;
                px.clamp(0, 1);
                intersect = true;
            }

            if (!intersect) {
                px = background;
            }

            *(img_ptr++) = px;
        }
    }


    // write image to file
    img_ptr = img;
    for (unsigned int i = 0; i<W*H; ++i) {
        Color c = *(img_ptr++) * MAX_VAL;
        c.round();
        ofs << c;
    }


    delete[] img;
    delete[] zbuff;
}