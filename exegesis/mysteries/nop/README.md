Investigating whether NOP uses issue ports.

See the discussion in https://reviews.llvm.org/D48028.

The two examples compare the effect of using long-NOP vs ALU instructions:

```
incl %%ecx
incl %%ecx
decl %%ecx
decl %%ecx
```

```
incl %%ecx
nopl (%%ecx)
decl %%ecx
nopl (%%ecx)
```

If the NOPs use issue ports, the measured uops should be the same for both
blocks. However, on `haswell` we measure the following:


```
    1.995 unhalted_core_cycles:u
    0.501 uops_executed_port:port_0:u
    0.502 uops_executed_port:port_1:u
    0.005 uops_executed_port:port_2:u
    0.006 uops_executed_port:port_3:u
    0.006 uops_executed_port:port_4:u
    0.505 uops_executed_port:port_5:u
    0.503 uops_executed_port:port_6:u
    0.002 uops_executed_port:port_7:u
```

```
    3.988 unhalted_core_cycles:u
    0.960 uops_executed_port:port_0:u
    0.972 uops_executed_port:port_1:u
    0.005 uops_executed_port:port_2:u
    0.006 uops_executed_port:port_3:u
    0.006 uops_executed_port:port_4:u
    1.035 uops_executed_port:port_5:u
    1.046 uops_executed_port:port_6:u
    0.003 uops_executed_port:port_7:u
```

