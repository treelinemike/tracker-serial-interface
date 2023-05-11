% restart
close all; clear; clc;

PROBE_ID = '3CC3F000';  % pen probe coil
% PROBE_ID = '3D4C7400';   % Yuan's coil for localization

fser = serialport('COM7',115200,'DataBits',8,'FlowControl','none','StopBits',1,'Timeout',0.001);

pkt = [];
while(isempty(pkt))
    requestAuroraPacket(fser,PROBE_ID);
    pkt = getAuroraPacket(fser,0.2);
end

clear fser;

T_coil_to_aurora = eye(4);
T_coil_to_aurora(1:3,1:3) = quat2matrix(pkt(3:6));
T_coil_to_aurora(1:3,4) = pkt(7:9);
T_coil_to_aurora


 