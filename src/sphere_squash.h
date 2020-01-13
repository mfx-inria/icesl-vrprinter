
// computes the volume of a sphere segment
// see http://mathworld.wolfram.com/SphericalSegment.html
double sphere_segment_volume(double r, double h1, double h2)
{
  if (h1 > h2) std::swap(h1,h2);
  double h = h2 - h1;
  double a = r * sqrt(r*r - h1*h1);
  double b = r * sqrt(r*r - h2*h2);
  std::cout << " a = " << a << std::endl;
  std::cout << " b = " << b << std::endl;
  double v = 1.0/6.0*(double)M_PI*h*(3.0*a*a+3.0*b*b+h*h);
  return v;
}

// computes the volume of a sphere cap
// h is height from sphere top
// see http://mathworld.wolfram.com/SphericalSegment.html
double sphere_cap_volume(double r, double h)
{  
  return (1.0/3.0)*(float)M_PI*h*h*(3.0*r-h);
}

double sphere_volume(double r)
{
  return (4.0/3.0)*(double)M_PI*r*r*r;
}

// finds the radius giving the same volume to a squashed
// sphere (clipped at h, where h is height from center) 
// as the volume of the sphere of radius r
float sphere_squashed_radius(float r,float h)
{
  // we search for rs such that:
  // sphere_volume(rs) - sphere_cap_volume(rs,rs-h)*2 == sphere_volume(r)
  // 4/3 pi rs^3 - 2/3 pi (rs-h)^2 * (3 rs - (rs-h)) == 4/3 pi r^3
  // 4 pi rs^3 - 2 pi (rs^2 + h^2 - 2 h rs) * (2 rs + h) == 4 pi r^3
  // 4 pi rs^3 - 2 pi ( 2 rs^3 + 2 h^2 rs - 4 h rs^2 + h rs^2 + h^3 - 2 h^2 rs) = 4 pi r^3

  // ( 3 h rs^2 - h^3 ) = 2 r^3
  // 3 h rs^2 = 2 r^3 + h^3
  // rs^2 = (2 r^3 + h^3) / (3 h)
  // rs = sqrt( (2 r^3 + h^3) / (3 h) )
  return sqrt( (2.0f *r*r*r + h*h*h) / (3.0f*h) );
}

// area of a disk
double disk_area(double r)
{
  return M_PI*r*r;
}

// area of a disk cap, height from top
// http://mathworld.wolfram.com/CircularSegment.html
double disk_cap_area(double r, double h)
{
  return r*r*acos((r-h)/r) - (r-h)*sqrt(2*r*h-h*h);
}

// finds the radius giving the same area to a squashed
// disk (clipped at h, where h is height from center) 
// as the area of the disk of radius r
double disk_squashed_radius(double r, double h)
{
  // we search for rs such that:
  // disk_area(rs) - disk_cap_area(rs,rs-h)*2 == disk_area(r)
  double L = 0.0;
  double R = r * 10.0; // assume this is enough?
  double aT = disk_area(r);
  while (fabs(L-R)>1e-6) {
    double m = (L + R)*0.5;
    double a = disk_area(m) - disk_cap_area(m, m - h) * 2.0;
    if (a > aT) {
      R = m;
    } else {
      L = m;
    }
  }
  return R;
}

/*
// for testing on e.g. http://cpp.sh/
 
int main()
{
  std::cout << sphere_segment_volume(1.0f,0.7f,1.0f) << "\n";
  std::cout << sphere_cap_volume(1.0f,1.0f-0.7f) << "\n";
  
  float r = 1.0f;
  float h = 0.1f;
  float rs = sphere_squashed_radius(r,h);
  std::cout << "rs = " << rs << std::endl;
  std::cout << "sphere_volume(r) = " << sphere_volume(r) << std::endl;
  std::cout << "sphere_volume(rs) - sphere_cap_volume(rs,rs-h)*2 = " << sphere_volume(rs) - sphere_cap_volume(rs,rs-h)*2 << std::endl;
}
*/