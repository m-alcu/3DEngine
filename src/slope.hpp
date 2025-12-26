#pragma once  

template<typename vertex>
class Slope  
{  
   vertex begin, step;

public:  
   Slope() {}  
   Slope(vertex from, vertex to, int num_steps)
   {  
       float inv_step = 1.f / num_steps;  
       begin = from;                   // Begin here  
       step = (to - from) * inv_step; // Stepsize = (end-begin) / num_steps  
   }  
   vertex get() const { return begin; }
   int getx() const { return begin.p_x >> 16; }  
   void advance() { begin += step; }  
};