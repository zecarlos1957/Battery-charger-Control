#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <string>



#define MAX_POINTS  600


using std::string;



 template <unsigned MOD>
class CircCount
{
     int val;
     void inc(){if(++val == MOD) val = 0;}
     void dec() {if(val--==0) val = MOD-1;}
     void normalize()
     {
         if(val>=int(MOD))val%=MOD;
         else while(val<0) val+=MOD;
     }
public:
     CircCount():val(0){}
     CircCount(unsigned v): val(v){ }
     void operator=(unsigned v){val=v;}
     operator unsigned()const{return val;}
     CircCount &operator++(){inc();return *this;}
     CircCount &operator--(){dec();return *this;}
     CircCount operator++(int){int v=val;inc();return v;}
     CircCount operator--(int){int v=val;dec();return v;}
     CircCount &operator+=(int v){val+=v;normalize();return *this;}
     CircCount &operator-=(int v){val-=v;normalize();return *this;}
     CircCount operator+(int v)const{CircCount c(*this);c+=v;return c;}
     CircCount operator-(int v)const{CircCount c(*this);c-=v;return c;}
     CircCount operator-(const CircCount &c)const
     { return val>=c.val?val-c.val:MOD-c.val+val;  }
     CircCount operator-()const{return MOD-val;}

};



       typedef CircCount<MAX_POINTS>  CountIterator;


class RingBuffer
{

      CountIterator start,finish;
      short val[MAX_POINTS];


public:
     RingBuffer():start(0),finish(0)
     { }
     ~RingBuffer(){}
      void pop_front(){ ++start; }
      void push_back(short p){
           DWORD n;
           val[finish]=p;
           if(++finish == start)
               pop_front();
      }
      CountIterator begin(){return start;}
      CountIterator end(){return finish;}
      short operator[](CountIterator idx)const { return val[idx];}
      short size()const{return finish-start;}
      bool empty()const{return start == finish;}
      bool IsFull(){ return size() == MAX_POINTS-1;}
 };



#endif
