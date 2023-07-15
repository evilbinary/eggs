
extern "C"{
    
int
__clzsi2 (int val)
{
  int i = 32;
  int j = 16;
  int temp;

  for (; j; j >>= 1)
    {
      if (temp = val >> j)
	{
	  if (j == 1)
	    {
	      return (i - 2);
	    }
	  else
	    {
	      i -= j;
	      val = temp;
	    }
	}
    }
  return (i - val);
}

int
__clzdi2 (long long val)
{
  if (val >> 32)
    {
      return __clzsi2 (val >> 32);
    }
  else
    {
      return __clzsi2 (val) + 32;
    }
}



double __aeabi_f2d(float f)
{
    return static_cast<double>(f);
}


// int __aeabi_idivmod(int numerator, int denominator)
// {
//     int quotient = numerator / denominator;
//     int remainder = numerator % denominator;
//     return (quotient << 16) | (remainder & 0xFFFF);
// }


// int __aeabi_idiv(int numerator, int denominator)
// {
//     int quotient = numerator / denominator;
//     return quotient;
// }

// #include "unwind.h"

// /* ABI defined personality routine entry points.  */

// #define __gnu_unwind_pr_common 

// _Unwind_Reason_Code
// __aeabi_unwind_cpp_pr0 (_Unwind_State state,
// 			_Unwind_Control_Block *ucbp,
// 			_Unwind_Context *context)
// {
//   return __gnu_unwind_pr_common (state, ucbp, context, 0);
// }

// _Unwind_Reason_Code
// __aeabi_unwind_cpp_pr1 (_Unwind_State state,
// 			_Unwind_Control_Block *ucbp,
// 			_Unwind_Context *context)
// {
//   return __gnu_unwind_pr_common (state, ucbp, context, 1);
// }

// _Unwind_Reason_Code
// __aeabi_unwind_cpp_pr2 (_Unwind_State state,
// 			_Unwind_Control_Block *ucbp,
// 			_Unwind_Context *context)
// {
//   return __gnu_unwind_pr_common (state, ucbp, context, 2);
// }


// _Unwind_Reason_Code 
// _Unwind_RaiseException(struct _Unwind_Exception *exc)
// {

// }

// void
// _Unwind_DeleteException (struct _Unwind_Exception *exc)
// {

// }
// void 
// _Unwind_Resume (struct _Unwind_Exception *exc)
// {

// }


// _Unwind_VRS_Result _Unwind_VRS_Get (_Unwind_Context *context,
// 				    _Unwind_VRS_RegClass regclass,
// 				    _uw regno,
// 				    _Unwind_VRS_DataRepresentation representation,
// 				    void *valuep)
// {


// }

// _Unwind_VRS_Result _Unwind_VRS_Set (_Unwind_Context *context,
// 				    _Unwind_VRS_RegClass regclass,
// 				    _uw regno,
// 				    _Unwind_VRS_DataRepresentation representation,
// 				    void *valuep)
// {

// }


// void *
// _Unwind_GetLanguageSpecificData (struct _Unwind_Context *context)
// {
  
// }

// _Unwind_Ptr
// _Unwind_GetRegionStart (struct _Unwind_Context *context)
// {
  
// }




}