Init 100 |+>;MultiBodyMeasure 0:X,4:X;MultiBodyMeasure 5:Z,6:Z;MultiBodyMeasure 2:Z,3:Z;RotateSingleCellPatch 100;
BusyRegion (2,5),(2,6),StepsToClear(1);
BusyRegion (2,5),(2,6),StepsToClear(0);HGate 1;
HGate 1;RotateSingleCellPatch 1;
BusyRegion (1,2),(1,3),StepsToClear(1);
BusyRegion (1,2),(1,3),StepsToClear(0);

