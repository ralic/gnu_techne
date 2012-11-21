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

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <objc/Object.h>

@interface Object (Base)

+(id) initialize;
-(void) init;

+(id) new;
+(id) alloc;
-(void) free;
-(id) copy;

-(const char *) name;

+(Class) superclass;
-(BOOL)isKindOf:(Class)aClassObject;
-(BOOL)isMemberOf:(Class)aClassObject;
+(BOOL)instancesRespondTo:(SEL)aSel;
-(BOOL)respondsTo:(SEL)aSel;

@end

#endif
