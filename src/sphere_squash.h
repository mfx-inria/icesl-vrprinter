
// computes the volume of a sphere segment
// see http://mathworld.wolfram.com/SphericalSegment.html
float sphere_segment_volume(float r,float h1, float h2)
{
  if (h1 > h2) std::swap(h1,h2);
  float h = h2 - h1;
  float a = r * sqrt(r*r - h1*h1);
  float b = r * sqrt(r*r - h2*h2);
  std::cout << " a = " << a << std::endl;
  std::cout << " b = " << b << std::endl;
  float v = 1.0f/6.0f*(float)M_PI*h*(3.0*a*a+3.0*b*b+h*h);
  return v;
}

// computes the volume of a sphere cap
// h is height from sphere top
// see http://mathworld.wolfram.com/SphericalSegment.html
float sphere_cap_volume(float r,float h)
{  
  return (1.0f/3.0f)*(float)M_PI*h*h*(3.0f*r-h);
}

float sphere_volume(float r)
{
  return (4.0f/3.0f)*(float)M_PI*r*r*r;
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
float disk_area(float r)
{
  return (float)M_PI*r*r;
}

// area of a disk cap, height from top
// http://mathworld.wolfram.com/CircularSegment.html
float disk_cap_area(float r,float h)
{
  return r*r*acos((r-h)/r) - (r-h)*sqrt(2*r*h-h*h);
}

// finds the radius giving the same area to a squashed
// disk (clipped at h, where h is height from center) 
// as the area of the disk of radius r
float disk_squashed_radius(float r, float h)
{
  // we search for rs such that:
  // disk_area(rs) - disk_cap_area(rs,rs-h)*2 == disk_area(r)
  float L = 0.0f;
  float R = r * 10.0f; // assume this is enough?
  float aT = disk_area(r);
  while (fabs(L-R)>1e-6f) {
    float m = (L + R)*0.5f;
    float a = disk_area(m) - disk_cap_area(m, m - h) * 2;
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