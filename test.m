rundata.data = ones(2, 3, 4, 5);
rundata.mask = ones(2, 3, 4);
rundata.model='poly';
rundata.degree = 3;
rundata.method='vb';
rundata.noise='white';

output = fabber(rundata)

% Show output log
output.log;

% Get an output image
mean_c0 = output.mean_c0.img;
size(mean_c0)
