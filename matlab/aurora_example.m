% restart
close all; clear; clc;

PORT_ID_SPINE    = '0A';   % spine coil
PORT_ID_PENCOIL  = '0B';   % pen probe coil

fser = serialport('COM13',115200,'DataBits',8,'FlowControl','none','StopBits',1,'Timeout',0.001);

all_tfs = getAuroraTransforms(fser,0.2,{PORT_ID_PENCOIL PORT_ID_SPINE});

clear fser;

 