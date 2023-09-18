% restart
close all; clear; clc;

% Define PORT HANDLE SRINGS for each tool, following AURORA convention:
% "0A" through "0D" left to right, starting with first connected TOOL...
%
% Examples:
%  (SPINE_COIL) (PEN_COIL) (X) (X)  --> SPINE_COIL = 0A, PEN_COIL = 0B
%  (SPINE_COIL) (X) (PEN_COIL) (X)  --> SPINE_COIL = 0A, PEN_COIL = 0B
%  (X) (SPINE_COIL) (X) (PEN_COIL)  --> SPINE_COIL = 0A, PEN_COIL = 0B
%  (X) (X) (SPINE_COIL) (PEN_COIL)  --> SPINE_COIL = 0A, PEN_COIL = 0B
%
PORT_ID_SPINE    = '0A';   % spine coil
PORT_ID_PENCOIL  = '0B';   % pen probe coil

% open serial port
fser = serialport('COM13',115200,'DataBits',8,'FlowControl','none','StopBits',1,'Timeout',0.001);

% request transforms
all_tfs = getAuroraTransforms(fser,0.25,{PORT_ID_PENCOIL PORT_ID_SPINE});

% get IDs
idarr = {all_tfs.sn};

% show spine TF
spine_idx = find(strcmp(idarr,PORT_ID_SPINE));
if(isempty(spine_idx))
    warning("Spine coil not found!");
else
    TF_spine_to_aurora = all_tfs(spine_idx).T_coil_to_aurora
end

% show pen probe TF
pen_idx = find(strcmp(idarr,PORT_ID_PENCOIL));
if(isempty(pen_idx))
    warning("Pen coil not found!");
else
    TF_pen_to_aurora = all_tfs(pen_idx).T_coil_to_aurora
end

% close serial port
clear fser;