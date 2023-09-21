// SPDX-License-Identifier: GPL-3.0

pragma solidity >=0.8.2 <0.9.0;

/**
 * This is a PCN demo for research purpose only
 */
contract PCN {

    uint32 public N;
    uint32 public L;
    uint32 public D;
    uint32[][] public adj;
    address payable[] public addr;

    uint32 public iter;
    uint32 public channelPerIndex;
    uint32 public custimizedChannelPerIndex;
    uint32 public dutyChannelPerIndex;
    uint256 public amountPerChannel;
    uint32[] public remainedCustimizedChannels;

    mapping(uint32 => mapping(uint32 => uint256)) public capacity;

    constructor (
        uint32 len, 
        uint32 d, 
        uint32 channel_per_index, 
        uint32 custimized_channel_per_index, // this is (1 - gamma) * m in the paper
        uint256 amount_per_channel // this is Q in the paper
    )  
    {
        require(custimized_channel_per_index <= channel_per_index);
        require(channel_per_index % d == 0);
        require(2 ** ((channel_per_index / d) - 2) <= len);
        require(2 ** ((channel_per_index / d) - 1) > len);
        // Note: channelPerIndex = (logL/log2 + 2) * D

        N = len ** d;    // `**' means power for Solidity
        L = len;
        D = d;

        iter = 0;
        adj = new uint32[][](N);  
        addr = new address payable[](N);

        amountPerChannel = amount_per_channel;
        channelPerIndex = channel_per_index;
        custimizedChannelPerIndex = custimized_channel_per_index;
        dutyChannelPerIndex = channelPerIndex - custimizedChannelPerIndex;

        remainedCustimizedChannels = new uint32[](N);
    }

    function corrdinateToIndex (uint32[] memory coor) 
        public 
        view 
        returns (uint32) 
    {
        uint32 res = 0;
        for(uint32 i = 0; i < D; i++)
            res += coor[i] * uint32(L ** (D - i - 1));
        return res;
    }

    function indexToCoordinate (uint32 index) 
        public 
        view 
        returns (uint32[] memory coor)
    {
        coor = new uint32[] (D);
        for(uint32 i = 0; i < D; i++)
            coor[i] = (index / uint32(L ** (D - i - 1))) % L;
        return coor;
    }

    function toGray (uint32 base, uint32 digits, uint32 value) 
        public 
        pure 
        returns (uint32[] memory gray)
    {
        // See https://en.wikipedia.org/wiki/Gray_code for explanations
        // Ref: Guan, Dah-Jyh (1998). "Generalized Gray Codes with Applications". Proceedings of the National Scientific Council, Taiwan, Part A. 22: 841–848. CiteSeerX 10.1.1.119.1344.
        uint32[] memory bn;
        bn = new uint32[](digits);
        uint32 i;
        for(i = 0; i < digits; i++) {
            bn[i] = value % base;
            value = value / base;
        }
        uint32 s = 0;
        gray = new uint32[](digits);
        for(i = digits - 1; i > 0; i--) {
            gray[i] = (bn[i] + s) % base;
            s = s + base - gray[i]; 
        }
        gray[0] = (bn[0] + s) % base;
        return gray;    // this line can be omitted for Solidity, but we keep it here for understanding
    }

    function registerIn ()
        public 
        payable 
        returns (uint32, uint32[] memory)
    {
        require(msg.value >= amountPerChannel * channelPerIndex);   // deposits for both prescribed channels and custimized channels
        uint32[] memory coor = toGray(L, D, iter);
        iter ++;
        uint32 index = corrdinateToIndex(coor);
        addr[index] = payable(msg.sender);

        adj[index] = new uint32[](dutyChannelPerIndex);
        uint adjPointer = 0;

        bool dutyOn = true;

        uint32[] memory coorTo = new uint32[](D);
        for(uint32 i = 0; i < D; i++)
            coorTo[i] = coor[i];

        for(uint32 k = 0; k < D; k++) {
            coorTo[k] = (coor[k] + 1) % L;
            if(dutyOn) {
                adj[index][adjPointer] = corrdinateToIndex(coorTo);
                adjPointer ++;
            }
            if(adjPointer == dutyChannelPerIndex)
                dutyOn = false;

            coorTo[k] = (coor[k] + L - 1) % L;
            if(dutyOn) {
                adj[index][adjPointer] = corrdinateToIndex(coorTo);
                adjPointer ++;
            }
            if(adjPointer == dutyChannelPerIndex)
                dutyOn = false;

            coorTo[k] = coor[k];
        }

        for(uint32 k = 0; k < D; k++) {
            for(uint32 step = 2; step < L; step = step * 2) {
                coorTo[k] = (coor[k] + step) % L;
                if(dutyOn) {
                    adj[index][adjPointer] = corrdinateToIndex(coorTo);
                    adjPointer ++;
                }
                if(adjPointer == dutyChannelPerIndex)
                    dutyOn = false;
            }

            coorTo[k] = coor[k];
        }

        assert(adjPointer == dutyChannelPerIndex);

        for(uint32 i = 0; i < adjPointer; i++)
            capacity[index][adj[index][i]] = amountPerChannel;

        remainedCustimizedChannels[index] = custimizedChannelPerIndex;

        return (index, adj[index]);
    }

    function addCustimizedChannel (uint32 index, uint32 indexTo) 
        public 
    {
        require(addr[index] == msg.sender);
        require(remainedCustimizedChannels[index] > 0);
        adj[index].push(indexTo);
        capacity[index][indexTo] = amountPerChannel;
        remainedCustimizedChannels[index] --;
    }

    function queryIndexAddress (uint32 index) 
        public 
        view 
        returns (address)
    {
        require(adj[index].length > 0); 
        return addr[index];
    }

    function queryOutwardAdjacentIndices (uint32 index)
        public
        view 
        returns (uint32[] memory) 
    {
        require(adj[index].length > 0); 
        return adj[index];
    }

    function showCapacity (uint32 indexSender, uint32 indexReceiver) 
        public 
        view 
        returns (uint256) 
    {
        return capacity[indexSender][indexReceiver];
    }

    function queryPrescribedInwardAdjacentIndices (uint32 index) 
        public 
        view
        returns (uint32[] memory res) 
    {
        uint32[] memory coor = indexToCoordinate(index);

        uint32[] memory coorFrom = new uint32[](D);
        for(uint32 i = 0; i < D; i++)
            coorFrom[i] = coor[i];

        res = new uint32[](dutyChannelPerIndex);
        uint32 count = 0;

        for(uint32 k = 0; k < D; k++) {
            coorFrom[k] = (coor[k] + L - 1) % L;
            
            if(count < dutyChannelPerIndex) {
                res[count] = corrdinateToIndex(coorFrom);
                count ++;
            }

            coorFrom[k] = (coor[k] + 1) % L;

            if(count < dutyChannelPerIndex) {
                res[count] = corrdinateToIndex(coorFrom);
                count ++;
            }

            coorFrom[k] = coor[k];
        }

        for(uint32 k = 0; k < D; k++) {
            for(uint32 step = 2; step < L; step = step * 2) {
                coorFrom[k] = (coor[k] + L - step) % L;
                if(count < dutyChannelPerIndex) {
                    res[count] = corrdinateToIndex(coorFrom);
                    count ++;
                }
            }

            coorFrom[k] = coor[k];
        }

        return res;
    }

    function topUp (uint32 indexSender, uint32 indexReceiver, uint256 value) 
        public 
        payable 
    {
        require(addr[indexSender] == msg.sender);
        require(value <= msg.value);
        capacity[indexSender][indexReceiver] += value;
    }

    function liquidate (uint32 indexSender, uint32 indexReceiver, address senderAddr, uint256 amount, bytes memory signature) 
        public 
    {
        require(amount <= capacity[indexSender][indexReceiver]);
        require(addr[indexSender] == senderAddr);
        require(addr[indexReceiver] == msg.sender);
        require(isValidSignature(senderAddr, amount, signature));
        uint32 len = uint32(adj[indexSender].length);
        bool found = false;
        for(uint i = 0; i < len; i++) {
            if(adj[indexSender][i] == indexReceiver) {
                found = true;
                adj[indexSender][i] = adj[indexSender][len - 1];
                adj[indexSender].pop(); // channel removed thereafter
            }
        }
        require(found); 
        addr[indexReceiver].transfer(amount);
        capacity[indexSender][indexReceiver] -= amount;
    }

    function isValidSignature(address senderAddr, uint256 amount, bytes memory signature)
        internal
        view
        returns (bool)
    {
        bytes32 message = prefixed(keccak256(abi.encodePacked(this, amount)));
        return recoverSigner(message, signature) == senderAddr;
    }

    
    function splitSignature(bytes memory sig)
        internal
        pure
        returns (uint8 v, bytes32 r, bytes32 s)
    {
        require(sig.length == 65);
        assembly {
            r := mload(add(sig, 32))
            s := mload(add(sig, 64))
            v := byte(0, mload(add(sig, 96)))
        }
        return (v, r, s);
    }

    function recoverSigner(bytes32 message, bytes memory sig)
        internal
        pure
        returns (address)
    {
        (uint8 v, bytes32 r, bytes32 s) = splitSignature(sig);
        return ecrecover(message, v, r, s);
    }


    function prefixed(bytes32 hash) internal pure returns (bytes32) {
        return keccak256(abi.encodePacked("\x19Ethereum Signed Message:\n32", hash));
    }

}
