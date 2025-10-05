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
        int numGates = default(3);
        @display("p=$x,$y");
        @class(Node);

    gates:
        inout gate[numGates];
       
}

module MobileNode {
    
    parameters:
        double x;
        double y;
        double range = default(300);
        int numGates = default(3);
        @display("p=1750, 300;i=block/wheelbarrow");
        
    gates:
        inout gate[numGates];
        
    submodules:
        mobility: TurtleMobility;
        node: Node {
            x = parent.x;
            y = parent.y;
            range = parent.range;
            numGates = parent.numGates;
        }
        
    connections:
        for i=0..numGates-1 {
            gate[i] <--> node.gate[i];
        }
}

network GarbageInTheCanAndSlow
{
    @display("bgb=3000,1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
    @figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);

    submodules:
        host: MobileNode {
            x = 1750;
            y = 300;
            range = 300;
            numGates = 3;
        }
        can: Node {
            x = 500;
            y = 150;
            range = 320;
            numGates = 1;
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
            range = 320;
            numGates = 1;
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            x = 1900;
            y = 650;
            range = 1600;
            numGates = 1;
            @display("p=1900,650;i=device/server");
        }
        
    connections:
        host.gate[0] <--> can.gate[0];
        host.gate[1] <--> anotherCan.gate[0];
    	host.gate[2] <--> cloud.gate[0];
}

network GarbageInTheCanAndFast
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: MobileNode {
            x = 1750;
            y = 300;
            range = 300;
            numGates = 2;
        }
        can: Node {
            x = 500;
            y = 150;
            range = 200;
            numGates = 2;
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
            range = 200;
            numGates = 2;
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            x = 1900;
            y = 650;
            range = 400;
            numGates = 2;
            @display("p=1900,650;i=device/server");
            
        }
    connections:
        host.gate[0] <--> can.gate[0];
        host.gate[1] <--> anotherCan.gate[0];
        can.gate[1] <--> cloud.gate[0];
        anotherCan.gate[1] <--> cloud.gate[1];
}

network NoGarbageInTheCans
{
    @display("bgb=3000, 1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
	@figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        host: MobileNode {
            x = 1750;
            y = 300;
            range = 300;
            numGates = 2;
        }
        can: Node {
            x = 500;
            y = 150;
            range = 200;
            numGates = 1;
            @display("p=500,150;i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
            range = 200;
            numGates = 1;
            @display("p=573.885,754.65;i=block/bucket");
        }
        cloud: Node {
            x = 1900;
            y = 650;
            range = 400;
            numGates = 0;
            @display("p=1900,650;i=device/server");
            
        }

    connections:
        host.gate[0] <--> can.gate[0];
        host.gate[1] <--> anotherCan.gate[0];
}
