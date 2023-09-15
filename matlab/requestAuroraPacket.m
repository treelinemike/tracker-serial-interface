function requestAuroraPacket(fser,probe_sn_array)

% definitions
DLE_BYTE = 0x10;
STX_BYTE = 0x02;
ETX_BYTE = 0x03;
GET_PROBE_TFORM = 0x13;

% assemble payload
payload = [];
payload(end+1) = GET_PROBE_TFORM;  % packet type
payload(end+1) = uint8(length(probe_sn_array)); % sent as uint8_t

% % include serial numbers in little endian format
% for id_idx = 1:length(probe_sn_array)
%     this_probe_sn = uint32(probe_sn_array(id_idx));
%     for i = 0:3  % little endian, lowest byte first
%         payload(end+1) = uint8(bitshift(bitand(this_probe_sn,bitshift(0x000000ff,8*i)),-8*i));
%     end
% end

% add port handle string characters
for i = 1:length(probe_sn_array)
    payload_insert = [0x00 0x00 0x00 0x00];
    this_porthandle_string = probe_sn_array{i};
    assert(length(this_porthandle_string) == 2,"Port handle string length must be 2!");
    payload_insert(1:length(this_porthandle_string)) = this_porthandle_string;
    for j = 1:length(payload_insert)
        payload(end+1) = uint8(payload_insert(j));
    end
end

% compute CRC
CRC_BYTE = crcAddBytes(0,payload);
payload(end+1) = CRC_BYTE;  % CRC byte IS stuffed... (TODO: check this)

% assemble message with DLE stuffing
msg = [DLE_BYTE STX_BYTE]; % firs two bytes not stuffed
for i = 1:length(payload)
    msg(end+1) = payload(i);
    if(payload(i) == DLE_BYTE)  % handle DLE stuffing
        msg(end+1) = DLE_BYTE;
    end
end
msg = [msg DLE_BYTE ETX_BYTE];

write(fser,msg,"uint8");
end