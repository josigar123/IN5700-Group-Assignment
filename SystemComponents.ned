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

import inet.mobility.contract.IMobility;
import inet.mobility.single.TurtleMobility;

simple Node
{
    parameters:
        double x;
        double y;
        double range = default(300);
        string rangeColor = default("green");
        @display("p=$x,$y");
        @class(Node);

    gates:
        input in[];
        output out[];
        
}

module MobileNode {
    
    parameters:
        double x;
        double y;
        double range = default(300);
        @display("i=block/wheelbarrow");
        
    gates:
        input in[];
        output out[];
        
    submodules:
        mobility: TurtleMobility{
        	@display("p=100,100");
        }
        node: Node {
            x = parent.x;
            y = parent.y;
        	range = parent.range;
        }
        
    connections allowunconnected:
        for i=0..sizeof(in)-1 {
            in[i] --> node.in++;
        }
        for i=0..sizeof(out)-1 {
            node.out++ --> out[i];
        }

}

network GarbageInTheCanAndSlow
{
    @display("bgb=3000,1250");
	@display("a=none");
	

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
    @figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);

    submodules:
        host: MobileNode {
            x = 1750;
            y = 300;
            range = 300;
            mobility.constraintAreaMinX = 150m;
            mobility.constraintAreaMinY = 200m;
            mobility.constraintAreaMaxX = 1700m;
            mobility.constraintAreaMaxY = 1100m;
        }
        can: Node {
            x = 500;
            y = 150;
            range = 320;
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
            range = 320;
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            x = 1900;
            y = 650;
            range = 1600;
            @display("p=1900,650;i=device/server");
        }
        
    connections:
        host.out++ --> { @display("ls=green,1"); delay = 100ms; } --> can.in++;
        host.in++ <-- { @display("ls=green,1"); delay = 100ms; } <-- can.out++;
        host.out++ --> { @display("ls=green,1"); delay = 100ms; } --> anotherCan.in++;
        host.in++ <-- { @display("ls=green,1"); delay = 100ms; } <-- anotherCan.out++;
        host.out++ --> { @display("ls=green,1"); delay = 100ms; } --> cloud.in++;
        host.in++ <-- { @display("ls=green,1"); delay = 100ms; } <-- cloud.out++;
}

network GarbageInTheCanAndFast
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: Node {
            x = 1750;
            y = 300;
            range = 300;
            @display("p=1750, 300;i=block/wheelbarrow");
        }
        can: Node {
            x = 500;
            y = 150;
            range = 200;
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
            range = 200;
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            x = 1900;
            y = 650;
            range = 400;
            @display("p=1900,650;i=device/server");
            
        }
    connections:
        host.out++ --> {  delay = 100ms; } --> can.in++;
        host.in++ <-- {  delay = 100ms; } <-- can.out++;
        host.out++ --> {  delay = 100ms; } --> anotherCan.in++;
        host.in++ <-- {  delay = 100ms; } <-- anotherCan.out++;
        can.out++ --> {  delay = 100ms; } --> cloud.in++;
        can.in++ <-- {  delay = 100ms; } <-- cloud.out++;
        anotherCan.out++ --> {  delay = 100ms; } --> cloud.in++;
        anotherCan.in++ <-- {  delay = 100ms; } <-- cloud.out++;
}

network NoGarbageInTheCans
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: Node {
            x = 1750;
            y = 300;
            range = 300;
            @display("p=1750, 300;i=block/wheelbarrow");
        }
        can: Node {
            x = 500;
            y = 150;
            range = 200;
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
            range = 200;
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            x = 1900;
            y = 650;
            range = 400;
            @display("p=1900,650;i=device/server");
            
        }

    connections:
        host.out++ --> {  delay = 100ms; } --> can.in++;
        host.in++ <-- {  delay = 100ms; } <-- can.out++;
        host.out++ --> {  delay = 100ms; } --> anotherCan.in++;
        host.in++ <-- {  delay = 100ms; } <-- anotherCan.out++;
}




