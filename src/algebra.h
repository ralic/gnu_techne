/* Copyright (C) 2009 Papavasileiou Dimitris                             
 *                                                                      
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or    
 * (at your option) any later version.                                  
 *                                                                      
 * This program is distributed in the hope that it will be useful,      
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
 * GNU General Public License for more details.                         
 *                                                                      
 * You should have received a copy of the GNU General Public License    
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ALGEBRA_H_
#define _ALGEBRA_H_

#include <math.h>

#define t_dot_3(u, v) ((u)[0] * (v)[0] + (u)[1] * (v)[1] + (u)[2] * (v)[2])
#define t_normalize_3(v)			\
    {						\
	typeof(v[0]) m, *_v;			\
						\
	m = sqrt(t_dot_3(_v, _v));		\
	(_v)[0] /= m;				\
	(_v)[1] /= m;				\
	(_v)[2] /= m;				\
    }

#define t_cross(u, v_1, v_2)				\
    {							\
	typeof(u[0]) *_u = u;					\
	typeof(v_1[0]) *_v_1 = v_1;				\
	typeof(v_2[0]) *_v_2 = v_2;				\
							\
	_u[0] = _v_1[1] * _v_2[2] - _v_1[2] * _v_2[1];	\
	_u[1] = _v_1[2] * _v_2[0] - _v_1[0] * _v_2[2];	\
	_u[2] = _v_1[0] * _v_2[1] - _v_1[1] * _v_2[0];	\
    }

#define t_concatenate_4_4(C, A, B)					\
    {									\
	typeof(C[0]) *_C = C;						\
	typeof(A[0]) *_A = A;						\
	typeof(B[0]) *_B = B;						\
									\
	_C[0] = _A[0] * _B[0] + _A[1] * _B[4] + _A[2] * _B[8] + _A[3] * _B[12];	\
	_C[1] = _A[0] * _B[1] + _A[1] * _B[5] + _A[2] * _B[9] + _A[3] * _B[13];	\
	_C[2] = _A[0] * _B[2] + _A[1] * _B[6] + _A[2] * _B[10] + _A[3] * _B[14];	\
	_C[3] = _A[0] * _B[3] + _A[1] * _B[7] + _A[2] * _B[11] + _A[3] * _B[15];	\
									\
	_C[4] = _A[4] * _B[0] + _A[5] * _B[4] + _A[6] * _B[8] + _A[7] * _B[12];	\
	_C[5] = _A[4] * _B[1] + _A[5] * _B[5] + _A[6] * _B[9] + _A[7] * _B[13];	\
	_C[6] = _A[4] * _B[2] + _A[5] * _B[6] + _A[6] * _B[10] + _A[7] * _B[14];	\
	_C[7] = _A[4] * _B[3] + _A[5] * _B[7] + _A[6] * _B[11] + _A[7] * _B[15];	\
									\
	_C[8] = _A[8] * _B[0] + _A[9] * _B[4] + _A[10] * _B[8] + _A[11] * _B[12];	\
	_C[9] = _A[8] * _B[1] + _A[9] * _B[5] + _A[10] * _B[9] + _A[11] * _B[13];	\
	_C[10] = _A[8] * _B[2] + _A[9] * _B[6] + _A[10] * _B[10] + _A[11] * _B[14]; \
	_C[11] = _A[8] * _B[3] + _A[9] * _B[7] + _A[10] * _B[11] + _A[11] * _B[15]; \
									\
	_C[12] = _A[12] * _B[0] + _A[13] * _B[4] + _A[14] * _B[8] + _A[15] * _B[12]; \
	_C[13] = _A[12] * _B[1] + _A[13] * _B[5] + _A[14] * _B[9] + _A[15] * _B[13]; \
	_C[14] = _A[12] * _B[2] + _A[13] * _B[6] + _A[14] * _B[10] + _A[15] * _B[14]; \
	_C[15] = _A[12] * _B[3] + _A[13] * _B[7] + _A[14] * _B[11] + _A[15] * _B[15]; \
    }

#endif
