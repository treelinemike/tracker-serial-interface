function requestAuroraPacket(fser,probe_sn)

DLE_BYTE = hex2dec('0x10');
STX_BYTE = hex2dec('0x02');
ETX_BYTE = hex2dec('0x03');
GET_PROBE_TFORM = hex2dec('0x13');

PKT_TYPE = GET_PROBE_TFORM;
PKT_LENGTH = 4;

% assemble payload
payload = [];
payload(end+1) = PKT_TYPE;
payload(end+1) = bitand(uint16(PKT_LENGTH),hex2dec('0x00FF'));  % little endian, lowest byte first
payload(end+1) = bitshift(bitand(uint16(PKT_LENGTH),hex2dec('0xFF00')),-8);

assert(length(probe_sn) == 8,'Invalid probe ID length!');
id_chars = uint8([]);
for i = 4:-1:1  % little endian, lowest byte first
    payload(end+1) = hex2dec(probe_sn((2*(i-1)+1)+(0:1)));
end

% compute CRC
CRC_BYTE = crcAddBytes(0,payload);

% assemble message with DLE stuffing
msg = [DLE_BYTE STX_BYTE];

for i = 1:length(payload)
    msg(end+1) = payload(i);
    if(payload(i) == DLE_BYTE)
        msg(end+1) = DLE_BYTE;
    end
end
msg = [msg CRC_BYTE DLE_BYTE ETX_BYTE];

write(fser,msg,"uint8");
end