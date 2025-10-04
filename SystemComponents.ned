//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

simple Node
{
    gates:
        input in[];   // Array of input gates
        output out[]; // Array of output gates
}

network GarbageInTheCanAndSlow
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: Node {
            @display("p=1750, 300;i=block/wheelbarrow");
        }
        can: Node {
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            @display("p=1900,650;i=device/server");
            
        }
    connections:
        host.out++ --> {  delay = 100ms; } --> can.in++;
        host.in++ <-- {  delay = 100ms; } <-- can.out++;
        host.out++ --> {  delay = 100ms; } --> anotherCan.in++;
        host.in++ <-- {  delay = 100ms; } <-- anotherCan.out++;
        host.out++ --> {  delay = 100ms; } --> cloud.in++;
        host.in++ <-- {  delay = 100ms; } <-- cloud.out++;
}

network GarbageInTheCanAndFast
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: Node {
            @display("p=1750, 300;i=block/wheelbarrow");
        }
        can: Node {
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            @display("p=1900,650;i=device/server");
            
        }
    connections:
        host.out --> {  delay = 100ms; } --> can.in;
        host.in <-- {  delay = 100ms; } <-- can.out;
        host.out --> {  delay = 100ms; } --> anotherCan.in;
        host.in <-- {  delay = 100ms; } <-- anotherCan.out;
        can.out --> {  delay = 100ms; } --> cloud.in;
        can.in <-- {  delay = 100ms; } <-- cloud.out;
        anotherCan.out --> {  delay = 100ms; } --> cloud.in;
        anotherCan.in <-- {  delay = 100ms; } <-- cloud.out;
}

network NoGarbageInTheCans
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: Node {
            @display("p=1750, 300;i=block/wheelbarrow");
        }
        can: Node {
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            @display("p=1900,650;i=device/server");
            
        }

    connections:
        host.out --> {  delay = 100ms; } --> can.in;
        host.in <-- {  delay = 100ms; } <-- can.out;
        host.out --> {  delay = 100ms; } --> anotherCan.in;
        host.in <-- {  delay = 100ms; } <-- anotherCan.out;
}




