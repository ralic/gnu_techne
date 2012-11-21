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

#include <objc/objc.h>
#include <objc/runtime.h>
#include "object.h"

@implementation Object (Base)

+(id) initialize
{
    return self;
}

-(void) init
{
}

+(id) new
{
    id object;

    object = [self alloc];
    [object init];
    
    return object;    
}

+(id) alloc
{
    return class_createInstance(self, 0);
}

-(void) free
{
    object_dispose (self);
}

-(id) copy
{
    return object_copy (self, 0);
}

-(const char *) name;
{
    return object_getClassName(self);
}

+(Class) class
{
    return self;
}

+(Class) superclass
{
    return class_getSuperclass(self);
}

-(BOOL)isKindOf:(Class)aClassObject
{
    Class class;

    class = object_getClass(self);

    do {
	if (class == aClassObject) {
	    return YES;
	}

	class = class_getSuperclass(class);
    } while (class);

    return NO;
}

-(BOOL)isMemberOf:(Class)aClassObject;
{
    return object_getClass(self) == aClassObject;
}

-(BOOL)respondsTo:(SEL)aSel;
{
    return class_respondsToSelector(object_getClass(self), aSel);
}

+(BOOL)instancesRespondTo:(SEL)aSel
{
    return class_respondsToSelector(self, aSel);
}

@end
