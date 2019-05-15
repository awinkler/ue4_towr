The towr.bag files can be generated with towr, found here:
https://github.com/ethz-adrl/towr

Command to turn xpp bag files into CSV file data.txt (replace with the name of your towr.bag):
rostopic echo -b towr.bag -p /xpp/state_des > data.txt

These CSV files are then read in by Unreal to execute the motions.
