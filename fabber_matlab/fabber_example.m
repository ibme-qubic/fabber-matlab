% Example usage of the Fabber-Matlab API 
rundata.data = rand(10, 10, 10, 5);
rundata.mask = ones(10, 10, 10);
rundata.model='poly';
rundata.degree = 3;
rundata.method='vb';
rundata.noise='white';
rundata.save_mean = true;

output = fabber(rundata)

% Show output log
output.log;

% Get an output image
mean_c0 = output.mean_c0.img;
size(mean_c0)
