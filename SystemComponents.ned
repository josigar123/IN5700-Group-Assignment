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

simple Visualizer
{
    @display("bgb=800,500");
    @figure[box](type=rectangle; pos=10,50; size=200,100; fillColor=green);
    @figure[box.label](type=text; pos=20,80; text=placeholder);
}

network GarbageInTheCanAndSlow
{
    @display("bgb=1400,500");
    @figure[box](type=rectangle; pos=10,50; size=200,100; fillColor=green);
    @figure[box.label](type=text; pos=20,80; text="placeholder");

    submodules:
        host: Node {
            @display("p=808,320");
        }
        can: Node {
            @display("p=396,176");
        }
        anotherCan: Node {
            @display("p=416,327");
        }
        cloud: Node {
            @display("p=924,185");
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
    submodules:
        host: Node {
            @display("p=399,180");
        }
        can: Node {
            @display("p=421,63");
        }
        anotherCan: Node {
            @display("p=405,300");
        }
        cloud: Node {
            @display("p=880,229");
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
    submodules:
        host: Node {
            @display("p=792,131");
        }
        can: Node {
            @display("p=421,63");
        }
        anotherCan: Node {
            @display("p=405,300");
        }
        cloud: Node {
            @display("p=930,184");
        }

    connections:
        host.out --> {  delay = 100ms; } --> can.in;
        host.in <-- {  delay = 100ms; } <-- can.out;
        host.out --> {  delay = 100ms; } --> anotherCan.in;
        host.in <-- {  delay = 100ms; } <-- anotherCan.out;
}




