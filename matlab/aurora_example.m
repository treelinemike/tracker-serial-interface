% restart
close all; clear; clc;

% PROBE_ID = '3CC3F000';  % pen probe coil
PROBE_ID = '3D4C7400';   % Yuan's coil for localization

fser = serialport('COM7',115200,'DataBits',8,'FlowControl','none','StopBits',1,'Timeout',0.001);

pkt = [];
while(isempty(pkt))
    requestAuroraPacket(fser,PROBE_ID);
    pkt = getAuroraPacket(fser,0.2);
end
pkt

clear fser;
 