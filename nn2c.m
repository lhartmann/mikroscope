function nn2c(nw)

inputs = nw.inputs{1}.size;
layers = nw.numLayers;

code = sprintf('void nn_eval(const double *X, double *Y) {\n');
%code = [code sprintf('')];

% Process layer math
code = [code sprintf('\t// Layer 1\n')];
code = [code sprintf('\tdouble S%d[%d];\n', 1, nw.layers{1}.size)];
for n=1:nw.layers{1}.size
	code = [code sprintf('\tS1[%d] = tanh(', n-1)];
	for i=1:inputs
		code = [code sprintf(' + %f * X[%d]', nw.iw{1}(n,i), i-1)];
	end
	code = [code sprintf(');\n')];
end
code = [code sprintf('\n')];
for l=2:layers
	code = [code sprintf('\t// Layer %d:\n', l)];
	code = [code sprintf('\tdouble S%d[%d];\n', l, nw.layers{l}.size-1)];
	for n=1:nw.layers{l}.size
		code = [code sprintf('\tS%d[%d] = tanh(', l, n-1)];
		for i=1:nw.layers{l-1}.size
			code = [code sprintf(' + %f * S%d[%d]', nw.lw{l,l-1}(n,i), l-1, i-1)];
		end
		code = [code sprintf(');\n')];
	end
	code = [code sprintf('\n')];
end
for n=1:nw.layers{layers}.size
	code = [code sprintf('\tY[%d] = S%d[%d];\n', n-1, layers, n-1)];
end
code = [code sprintf('}\n\n')];



disp(code);
