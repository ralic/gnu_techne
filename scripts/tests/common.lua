-- Copyright (C) 2010-2011 Papavasileiou Dimitris                           
--                                                                      
-- This program is free software: you can redistribute it and/or modify 
-- it under the terms of the GNU General Public License as published by 
-- the Free Software Foundation, either version 3 of the License, or    
-- (at your option) any later version.                                  
--                                                                      
-- This program is distributed in the hope that it will be useful,      
-- but WITHOUT ANY WARRANTY; without even the implied warranty of       
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
-- GNU General Public License for more details.                         
--                                                                      
-- You should have received a copy of the GNU General Public License    
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

techne.iterate = true

graphics.window = {800, 600}
graphics.title = 'Foo'
graphics.hide = false

bindings['escape'] = function ()
			techne.iterate = false
		     end

bindings['q'] = function ()
		   techne.iterate = false
		end

bindings['down-control_l c'] = function ()
   techne.interactive = true
end
