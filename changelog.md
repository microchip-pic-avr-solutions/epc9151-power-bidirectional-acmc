# epc9151-power-bidirectional-acmc, release v1.0.1

### Release Highlights
This initial provides two independent firmware projects running the board in Average Current Mode Control in step-down or step-up operation. This firmware may get merged into one comprehensive firmware package supporting both modes simultaneously.

### Features Added\Updated

##### Average Current Mode Control

An outer voltage loop regulates the output voltage by comparing the most recent feedback value against an internal reference. The deviation is processed by a discrete type II (2P2Z) compensation filter. The output of the voltage loop sets the reference for the two inner current loops. Each phase current controller processes the deviation between the given dynamic current reference and the individual most recent current feedback. Each current control loop output adjusts the individual duty cycle or phase resulting in tightly balanced phase currents. This control scheme is applied to both, 48 V to 12 V downstream buck as well as to 12 V to 48 V upstream boost operation.

When powered from a single DC source from either side of the converter, the output voltage will be kept constant up to the maximum output current of 25 A buck respectively. 5.5 A in boost operation, at which stage the converter switches into the constant current mode, effectively disables the voltage regulation.

This firmware serves as the fundamental building block of battery charger front-end systems by implementing a chemistry-specific charging profile or as balancing converter between two battery powered bus rails.



