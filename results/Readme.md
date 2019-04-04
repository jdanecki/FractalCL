Results in these directories are taken by executing
```
FractalCL -t -a -q
```
with FP_64_SUPPORT=OFF because not all OpenCL implementations provide working support for cl_khr_fp64 extension.

# Results summary

| Vendor | CPU       | GPU         | Driver          | Driver type | Result | 
| ------ | --------- | ----------- | --------------- | ----------- | ------ |
| Nvidia |           | GTX 1050Ti  | 390.67          | OCL GPU CUDA| 43     |
| Nvidia |           | Quadro M1200| 410.78          | OCL GPU CUDA| 96     |
| Intel  | i7-8700K  |             | 1.2.0.37        | OCL CPU     | 145    |
| Intel  | i7-8700K  | CFL GT2     | Neo 19.12.12612 | OCL GPU     | 170    |
| Intel  | i7-7820HQ | KBL GT2     | Neo 19.12.12612 | OCL GPU     | 213    |
| Intel  | i7-8700K  | CFL GT2     | Beignet 1.4     | OCL GPU     | 217    |
| Intel  | i5-6260U  | SKL GT3     | Neo 1.0         | OCL GPU     | 251    |
| Intel  | i5-6260U  | SKL GT3     | Beignet 1.2     | OCL GPU     | 265    |
| Nvidia |           | GTX 770     | 390.48          | OCL GPU CUDA| 270    |
| Intel  | i5-6500   | SKL GT2     | Neo 1.0         | OCL GPU     | 284    |
| Intel  | i7-7820HQ | KBL GT2     | Beignet 1.4     | OCL GPU     | 303    |
| Intel  | i5-4200M  | HSW GT2     | Beignet 1.4     | OCL GPU     | 353    |
| Intel  | i7-6770HQ | SKL GT4     | Neo 18.25.10965 | OCL GPU     | 499    |
| Intel  | i5-4200M  |             | 1.2.0.37        | OCL CPU     | 646    |
| Intel  | i7-7820HQ |             | pocl 1.1.4      | OCL CPU     | 724    |
| Intel  | i7-6770HQ | SKL GT4     | Beignet 1.1.1   | OCL GPU     | 1382   |
| Intel  | i5-4200M  | HSW GT2     | SRB5 r5.0.63503 | OCL GPU     | 3058   |
