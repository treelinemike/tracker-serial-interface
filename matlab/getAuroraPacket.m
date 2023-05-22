function pkt = getAuroraPacket(fser,timeout)

% get starting time to check for timeout
tic;  

% flush serial port
fser.flush();

% variables, etc.
newSerialData = [];
DLE_BYTE = hex2dec('0x10');
STX_BYTE = hex2dec('0x02');
ETX_BYTE = hex2dec('0x03');
PKT_TYPE_TRANSFORM_DATA = hex2dec('0x01');
PKT_LENGTH_TRANSFORM_DATA = 43;
PKT_TYPE_ACK = hex2dec('0x06');
PKT_TYPE_TRK_START = hex2dec('0x11');
PKT_TYPE_TRK_STOP = hex2dec('0x12');
PKT_TYPE_NAK = hex2dec('0x15');
serialMaxReadSize = 150;
messageMaxSize = 300;
finalDataMaxSize = 1e6;
pkt = double([]);
newSerialData = [];
startIdx = 0;

% start reading from serial port
% buffer until DLE, STX pattern is found
while(~startIdx && (toc < timeout))

    if(fser.NumBytesAvailable > 0)
        data = fser.read(fser.NumBytesAvailable,'uint8');
        newSerialData = [newSerialData, data];

        % search queue for start of message
        for i = 1:size(newSerialData,1)
            if( (newSerialData(i) == DLE_BYTE) && (length(newSerialData) > 1) && (newSerialData(i+1) == STX_BYTE) )
                % exclude case where (DLE, DLE, STX) pattern occurs in data
                % otherwise ready to start collecting data from serial port
                if((i == 1) || (newSerialData(i-1) ~= DLE_BYTE))
                    startIdx = i;
                    break;
                end
            end
        end

        % if we found the start of a message, discard all previous data
        if(startIdx)
            newSerialData = newSerialData(startIdx:end);
        end
    end
    %disp(['Waiting buffer size: ' num2str(length(newSerialData))]);
end


% newSerialData now contains DLE_BYTE and STX_BYTE as its first and second
% elements; enter a continuous loop of processing and reading more data
% from serial port (exit with CTRL-C)
%disp('Capturing data from serial port...');
while(isempty(pkt) && (toc < timeout)) % TODO: provide more elegant method of exiting loop (multi-threaded GUI?)
    
    msgsInBufferFlag = 1;
    
    while(msgsInBufferFlag)
        % find all occurances of DLE, ETX byte sequence in buffer
        DLEETXidx = strfind(newSerialData,[DLE_BYTE ETX_BYTE]);
        
        % keep only occurances where ETX is preceeded by an odd number of DLE bytes
        % as this signals end of message (DLE byte before ETX is not stuffed)
        newDLEETXidx = [];
        for i=1:length(DLEETXidx)
            numDLEs = 0;
            thisDLEETXidx = DLEETXidx(i);
            DLECountIdx = thisDLEETXidx;
            
            % count DLE bytes before this ETX byte
            while((DLECountIdx > 0) && (newSerialData(DLECountIdx) == DLE_BYTE))
                numDLEs = numDLEs + 1;
                DLECountIdx = DLECountIdx -1;
            end
            
            % if odd number of DLE bytes, this DLE, ETX is the end of a message
            if( mod(numDLEs,2) == 1 )
                newDLEETXidx(end+1) = thisDLEETXidx;
            end
        end
        DLEETXidx = newDLEETXidx;
        
        % signal to stop repeating processing loop if there is only one message
        % in buffer
        if(length(DLEETXidx) <= 1)
            msgsInBufferFlag = 0;
        end
        
        % parse a message if we have one
        if(~isempty(DLEETXidx))
            
            % first make sure DLE and STX are first two characters
            DLESTXidx = strfind(newSerialData,[DLE_BYTE STX_BYTE]);
            if(isempty(DLESTXidx) || (DLESTXidx(1) ~= 1))
                error('Data Format Error: Starting buffer sequence not DLE, STX');
                    % TODO: recover from this by discarding this message and
                    % waiting for next properly-formed message?
            end
            
            % extract message, skipping stuffed DLE bytes
            thisMsg = [];   % Note: memory allocation not really necessary in MATLAB
            thisMsgIdx = 0;
            i = 1;
            while(i < (DLEETXidx(1)+2))
                thisMsgIdx = thisMsgIdx + 1;
                if(thisMsgIdx > messageMaxSize)
                    error('thisMsgIdx overrun!');
                    % TODO: recover from this by discarding this message and
                    % waiting for next properly-formed message?
                end
                thisMsg(thisMsgIdx,1) = newSerialData(i);
                
                % if current and next byte are DLE, skip the next one (this was a stuffed DLE)
                if((newSerialData(i) == DLE_BYTE) && (newSerialData(i+1) == DLE_BYTE))
                    i = i+2;
                else
                    i = i+1;
                end
            end
            
            % get size of message
            thisMsgLength = thisMsgIdx;
            
            % remove bytes associated with this message from our queue
            newSerialData = newSerialData((DLEETXidx(1)+2):end);  % TODO: rewrite for speed, portability
            
            % TODO: COULD CHECK CRC HERE!
            
            % parse message (note: conversion to engineering units requires
            % knowledge of IMU settings, though raw data are also stored for
            % future reprocessing)
            pkt_type = double(uint8(thisMsg(3)));
            
            % process packet by type-specific methods
            switch(pkt_type)
                
                % process data packets
                case PKT_TYPE_TRANSFORM_DATA 
                    
                    % flag to continue through error checks
                    proceed = true;

                    num_tforms = uint8(thisMsg(4));


                    % check length
                    % TODO: include length as a field in protocol and/or check CRC8
                    if( thisMsgLength ~= 11+36*num_tforms )
                        warning(['invalid message length (' num2str(length(thisMsg)) ') for packet type 0x' dec2hex(pkt_type) '!' ]);
                        proceed = false;
                    end

                    % check CRC
                    if(proceed && ( crcAddBytes(0,thisMsg(3:end-3)) ~= thisMsg(end-2)) )
                        warning('CRC check failed!');
                        proceed = false;
                    end

                    % convert data and put into packet
                    if(proceed)
                        %fprintf('Received packet!\n');
                        pkt(1,1) = typecast(uint8(thisMsg(5:8)),'uint32');    % frame num
                        for tform_idx = double(0:(double(num_tforms)-1))
                            pkt(1,2+9*tform_idx) = typecast(uint8(thisMsg((9:12)+36*tform_idx)),'uint32');   % tool s/n
                            pkt(1,3+9*tform_idx) = typecast(uint8(thisMsg((13:16)+36*tform_idx)),'single');  % q0
                            pkt(1,4+9*tform_idx) = typecast(uint8(thisMsg((17:20)+36*tform_idx)),'single');  % q1
                            pkt(1,5+9*tform_idx) = typecast(uint8(thisMsg((21:24)+36*tform_idx)),'single');  % q2
                            pkt(1,6+9*tform_idx) = typecast(uint8(thisMsg((25:28)+36*tform_idx)),'single');  % q3
                            pkt(1,7+9*tform_idx) = typecast(uint8(thisMsg((29:32)+36*tform_idx)),'single');  % tx
                            pkt(1,8+9*tform_idx) = typecast(uint8(thisMsg((33:36)+36*tform_idx)),'single');  % ty
                            pkt(1,9+9*tform_idx) = typecast(uint8(thisMsg((37:40)+36*tform_idx)),'single');  % tz
                            pkt(1,10+9*tform_idx) = typecast(uint8(thisMsg((41:44)+36*tform_idx)),'single'); % error
                        end
                    end
                   
                otherwise
                    % TODO: PROCESS AT LEAST NAK PACKETS
                    warning(['Packet type 0x' dec2hex(mtype) ' not supported!']);
                    
            end  % end of switch(mType)
        end % end of if(~isempty(DLEETXidx))
    end % end of while(msgsInBuffer)
    
    % read more data from serial port once all messages in buffer have been
    % processed

    data = [];
    if(fser.NumBytesAvailable > 0)
        data = fser.read(fser.NumBytesAvailable,'uint8');
        newSerialData = [newSerialData, data];
    end
    
end % end of while()

if(toc > timeout)
    warning('Serial read timeout!');
end
end