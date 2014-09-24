/*
 * GraphTheoryTypes.h
 *
 *  Created on: 2014-01-08
 *      Author: sam
 */

#ifndef GEOMETRY_TYPES_H_
#define GEOMETRY_TYPES_H_
#include <initializer_list>
#include "core/SolverTypes.h"
#include "mtl/Rnd.h"
#include "mtl/Vec.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <gmpxx.h>
#include <iostream>
#include <utility>

using namespace Minisat;

/**
 * c++14 version:
 * template <typename T> constexpr T epsilon;

template <> constexpr float  epsilon<float>  = 0.001f;
template <> constexpr double epsilon<double> = 0.000001;
 */
template <typename T> struct epsilon;

template <> struct epsilon<float>
{
private:
	 float const value = 0.000001f;
public:

    operator double()const{return value;}
};

template <> struct epsilon<double>
{
private:
	 double const value = 0.000000001;
public:

     operator double ()const{return value;}
};


template <> struct epsilon<mpq_class>
{
private:
	 const mpq_class  value = mpq_class(0);
public:

	 operator mpq_class ()const{return value;}
};

template<typename T> struct numeric{
    static T inf;

    //Provides a useable 'infinity' value, which is (hopefully!) larger than any in use.
    //This wrapper is required for dealing with gmp, which doesn't currently provided inf/nan values.
	static T infinity() {
		if(std::numeric_limits<T>::has_infinity)
			return std::numeric_limits<T>::infinity();
		else{
			return inf;
		}
	}
};
template<typename T> T  numeric<T>::inf=std::numeric_limits<long>::max();//hopefully large enough for common use cases...

//This may have rounding errors! Use carefully!
inline mpq_class sqrt(mpq_class v){
	return (mpq_class) sqrt((mpf_class)v);
}


template<class T> inline bool equal_epsilon(T a, T b);
template<class T> inline bool eq_epsilon(T a){
	return std::abs(a)<=epsilon<T>();
}
template<class T> inline bool gt_epsilon(T a){
	return std::abs(a)>epsilon<T>();
}
template<> inline bool eq_epsilon(mpq_class a){
	return abs(a)<=(mpq_class)epsilon<mpq_class>();
}

template<> inline bool equal_epsilon(mpq_class a, mpq_class b){
	return a==b;//mpq_class is exact
	//return std::abs(a-b)<=epsilon<mpq_class>();
}
template<> inline bool equal_epsilon(double a, double b){
	return std::abs(a-b)<=epsilon<double>();
}
template<> inline bool equal_epsilon(float a, float b){
	return std::abs(a-b)<=epsilon<float>();
}

//NOTE: Points cannot be copied using realloc, and so cannot be stored in mtl::vec!
template<unsigned int D, class T>
struct Point{
	int size()const{
		return D;
	}
	int id=-1;//optional identifier variable
	T vector[D];
	T & x;
	T & y;
	T & z;

    // Vector interface:
    const T& operator [] (int index) const {assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]); assert(index<D);assert(index>=0); return vector[index];}
    T&       operator [] (int index)       {assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);assert(index<D);assert(index>=0);  return vector[index];}

    Point():id(-1),x(vector[0]),y(vector[1]),z(vector[2]){
    /*	for(int i = 0;i<D;i++){//this is not required; it is already built-in to the constructor.
    		new (&vector[i]) T();
    	}*/
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    }

    ~Point(){

    }

    Point(const std::vector<T> & list  ):id(-1),x(vector[0]),y(vector[1]),z(vector[2]){
    	assert(list.size()==D);
    	for(int i = 0;i<D;i++){
    		vector[i].~T();
			new (&vector[i]) T(list[i]);
    	}
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    }
    Point(const vec<T> & list  ):id(-1),x(vector[0]),y(vector[1]),z(vector[2]){
    	assert(list.size()==D);
    	for(int i = 0;i<D;i++){
    		vector[i].~T();
			new (&vector[i]) T(list[i]);
    	}
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    }
    //Copy constructor
    Point(const Point<D,T> & v):id(v.id),x(vector[0]),y(vector[1]),z(vector[2]){
    	assert(v.size()==D);
    	for(int i = 0;i<D;i++){
    		vector[i].~T();
    		new (&vector[i]) T(v[i]);
    	}
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    }
    //c++11 move constructor
     Point& operator=(Point&& rhs){
    	 if(this != & rhs){
    		 id=rhs.id;
    		 rhs.id = -1;
    		 for(int i = 0;i<D;i++){
    			 vector[i] =std::move(rhs.vector[i]);
    		 }
    	 }
    	 return *this;
     }
    Point( std::initializer_list<T> list ):id(-1),x(vector[0]),y(vector[1]),z(vector[2]){
    	assert(list.size()==size());
    	vector=list;
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    }
    template<typename... Ts>
    Point( Ts... args ):id(-1),vector{args...},x(vector[0]),y(vector[1]),z(vector[2]){
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    }
    Point& operator=(const Point & v)
    {
    	id=v.id;
    	for(int i = 0;i<D;i++){
    		vector[i].~T();
			new (&vector[i]) T(v[i]);
		}
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
      return *this;
    }



    int getID()const{
    	assert(hasID());
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    	return id;
    }
    bool hasID()const{
    	return id>=0;
    }
    void setID(int id){
    	this->id=id;
    }
    T dot(const Point<D,T> & other)const{
    	T sum=T();
    	for (int i = 0;i<D;i++){
    		sum += vector[i]*other[i];
    	}
    	assert(&x==&vector[0]);assert(&y==&vector[1]);assert(&z==&vector[2]);
    	return sum;
    }
    void zero(){
    	for(int i = 0;i<D;i++){
    		vector[i]=0;
    	}
    }

    Point<D,T>  operator-()const  {
      Point<D,T> p;
      for(int i = 0;i<D;i++){
			p[i]+= -(*this)[i];
	  }
      return p;
    }

    Point<D,T> &       operator += (const Point<D,T>& other) {
    	for(int i = 0;i<D;i++){
    		vector[i]+=other[i];
    	}
    	return *this;
    }
    Point<D,T> &       operator -= (const Point<D,T>& other) {
    	for(int i = 0;i<D;i++){
    		vector[i]-=other[i];
    	}
    	return *this;
    }
    Point<D,T> &       operator /= (const T & scalar) {
    	for(int i = 0;i<D;i++){
    		vector[i]/=scalar;
    	}
    	return *this;
    }
    Point<D,T> &       operator *= (const T & scalar) {
    	for(int i = 0;i<D;i++){
    		vector[i]*=scalar;
    	}
    	return *this;
    }
    Point<D,T>        operator * (const T & scalar)const {
    	Point<D,T> ret;
    	for(int i = 0;i<D;i++){
    		ret[i]= vector[i]* scalar;
    	}
    	return ret;
    }
    Point<D,T>        operator / (const T & scalar)const {
    	Point<D,T> ret;
    	for(int i = 0;i<D;i++){
    		ret[i]= vector[i]/ scalar;
    	}
    	return ret;
    }
    //Note that this computes a DOUBLE. Should enforce that is a safe underapproximation of the real distance between these points (that is, the actual distance is guaranteed to be >= the returned value)
    double distance_underapprox(const Point<D,T> other){
    	T sum=T(0);
    	for(int i =0;i<D;i++){
    		T difference = (vector[i]-other[i]);
    		sum+= difference*difference;
    	}
    	double distance = sqrt((double)sum);
    	return distance;
    }
};
namespace Minisat{
template<unsigned int D,class T>
class vec<::Point<D,T>>;
	//static_assert(false,"mtl vec cannot be used to store Points (as they are not reallocatable)");

};
template<unsigned int D, class T>
inline bool operator==(const Point<D,T>& lhs, const Point<D,T>& rhs){
	for(int i = 0;i<D;i++){
		if(lhs[i]!= rhs[i])
			return false;
	}
	return true;
}
template<unsigned int D, class T>
inline bool operator!=(const Point<D,T>& lhs, const Point<D,T>& rhs){
	return !(lhs==rhs);
}
template<unsigned int D, class T>
inline Point<D,T> operator+(const Point<D,T> &a, const Point<D,T> &b)
{
	Point<D,T> p;
	for(int i = 0;i<D;i++){
		p[i]=a[i]+b[i];
	}
    return p;
}

template<unsigned int D, class T>
inline Point<D,T> operator-(const Point<D,T> &a, const Point<D,T> &b)
{
	Point<D,T> p;
	for(int i = 0;i<D;i++){
		p[i]=a[i]-b[i];
	}
    return p;
}
template<unsigned int D,class T>
std::ostream & operator<<(std::ostream & str, Point<D,T> const & p) {
  str<<"(";
  for (int i =0;i<D-1;i++){
	  str<< p[i]<<",";
  }
  str<<p[p.size()-1] <<")";
  return str;
}

typedef Point<2,double> Point2D;
typedef Point<3,double> Point3D;

template<unsigned int D,class T>
struct SortBy{
	int sortOn;
	bool operator()(const Point<D,T> & a,const Point<D,T> & b)const{
		return a[sortOn]<b[sortOn];
	}
	SortBy(int dimensionToSort):sortOn(dimensionToSort){}
};

template<unsigned int D,class T>
struct SortLexicographic{

	bool operator()(const Point<D,T> & a,const Point<D,T> & b)const{
		for(int i =0;i<D;i++)
			if(a[i]<b[i])
				return true;
			else if (a[i]>b[i])
				return false;
		return false;
	}

};
template<class T>
static T crossDif(const Point<2,T> &O, const Point<2,T>  &A, const Point<2,T>  &B)
{
	return (A[0] - O[0]) * (B[1] - O[1]) - (A[1] - O[1]) * (B[0] - O[0]);
}

template<class T>
static T dotDif(const Point<2,T> &A, const Point<2,T>  &B, const Point<2,T>  &C)
{
	return (C[0] - A[0]) * (B[0] - A[0]) + (C[1] - A[1]) * (B[1] - A[1]);
}

template<class T>
static T cross2d(const Point<2,T> &A, const Point<2,T>  &B){
	return A.x*B.y - A.y*B.x;
}

enum class Winding{
	CLOCKWISE,COUNTER_CLOCKWISE, NEITHER
};

template<class T>
static Winding computeWinding(const std::vector<Point<2,T>> & points) {
	bool everCW = false;
	bool everCCW = false;
	T sum = 0;
	for(int i = 0;i<points.size();i++){
		const Point<2,T> & prev = i>0?points[i-1]:points.back();
		const Point<2,T> & p = points[i];

		sum +=(cross2d(prev,p)>0);
	}
	if(sum>0)
		return Winding::CLOCKWISE;
	else if (sum<0){
		return Winding::COUNTER_CLOCKWISE;
	}else if (sum==0){
		return Winding::NEITHER;
	}
}
template<class T>
static bool isClockwise(const std::vector<Point<2,T>> & points) {
	return computeWinding(points)!=Winding::COUNTER_CLOCKWISE;
}
template<class T>
static bool isCounterClockwise(const vec<Point<2,T>> & points) {
	return computeWinding(points)!=Winding::CLOCKWISE;
}
template<class T>
static bool isConvex(const std::vector<Point<2,T>> & points){
	if(!isClockwise(points)){
		return false;
	}
	bool seenPositive=false;
	bool seenNegative=false;
	for(int i = 0;i<points.size();i++){
		const Point<2,T> & prev = i>0?points[i-1]:points.back();
		const Point<2,T> & p = points[i];
		const Point<2,T> & next = i<points.size()-1 ?  points[i+1]:points[0];
		Point<2,T> a = p-prev;
		Point<2,T> b = next-p;
		T s = cross2d(a,b);
		seenPositive |= s>0;
		seenNegative |= s<0;
		if(seenPositive && seenNegative)
			return false;
	}
	return true;
}

template<class T>
static void randomShuffle(double& seed, T* start, T* end)
{
	int size = end-start;
    for (int i = 0; i < size; i++){
        int pick = i + irand(seed,size - i);
        std::swap( start[i], start[pick]);
    }
}

enum class PolygonOperationType{
	op_union, op_difference, op_intersect, op_minkowski_sum
};

template<unsigned int D, class T>
class GeometryTheorySolver;


#endif /* GEOMETRY_TYPES_H_ */
