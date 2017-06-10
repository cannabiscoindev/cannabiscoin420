// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (  0, uint256("0x00000a10f7ce671e773330376ce892a6c0b93fbc05553ebbf659b11e3bf9188d"))
        (  57600, uint256("0x0000000015fedc25afc3db164ef667cc7b86761e430ad2c8348178b35f3a7ae5"))
        (  115200, uint256("0x000000000d48cdef252c47317ff3ec976e6690d8fa16e736162f5660c210cb88"))
        (  172800, uint256("0x000000000029b8da63ad224f0af1d6ae1dda36df76685584cff7b8291425fff9"))
        (  230400, uint256("0x000000000197256fb0a4439f97c158781e4a0c6bbc50943789b6454f30d03737"))
        (  288000, uint256("0x00000000000689e15ee64d18d17bde40a55c9c739d2104487620d9c94fde49a6"))
        (  345600, uint256("0x000000000247734e6bf547ba4bfc0948df0854a8ec2a7e07a6424f9a2867847a"))
        (  403200, uint256("0x0000000004175725c085588b751f1680d02be94e3b620049e0653c1b99a2ad22"))
        (  460800, uint256("0x0000000003d7ffd06b7caa52abfb2b61857fc20023802d3f20c7bfa268e9f0c0"))
        (  518400, uint256("0x00000000050ee93ba705ebb3ebb0b0290d84fcbadc35b2ddfeeb9e2fc45fa9c6"))
        (  576000, uint256("0x000000000513e8d692fc15b90dc217ea6a2ecfb87f8f6008621043c42f11be18"))
        (  633600, uint256("0x000000000547c4c5b882b98bd472fef0417d1f66bab9a38e0b55310420aee65b"))
        (  691200, uint256("0x000000000fe9f5cafc96a1f3217033b4f37a52d1465c16bf866eac6cb6460950"))
        (  748800, uint256("0x0000000028f231274ddafdb2127f1e944685fd4a010a0990605616953690401a"))
        (  806400, uint256("0x000000000607e68c5758df6595f318e70ab1d0f5c6620a11a47873d7fe080686"))
        (  864000, uint256("0x0000000019689b58de02a327a7454ec7faa5cafc71f837bf0b1903386483a3a5"))
        (  921600, uint256("0x00000000456f8a90b5dbbe6b9ba95cf79262cfb51db87b1de517996bf7a9421c"))
        (  979200, uint256("0x000000006860a93401d32538d1454962b6f64834f005d9b3027e770b49a39bbc"))
        (  1036800, uint256("0x00000000a9a5fa171cc3cd81b46e8773d845153310e1a51c63e821537e751395"))
        (  1094400, uint256("0x000000002fac5cfff0c6efb1641662547c0d9046f455236beb8f094a4599dbee"))
        (  1121000, uint256("0x000000012b18631c9d5d90e3c5a32655f63ae1100ff8cafbde184521deba0960"))
        (  1135050, uint256("0x000000014d3dfb1e1a6cfa86d00baf64acd273da8536badc9aaf4f090a9b77af"))
		(  2210000, uint256("0x00000000339ff4df710efe0ff81f4c307343cba44a5a166412b30f764029ef76"))
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1496619975, // * UNIX timestamp of last checkpoint block
        2644312,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        2778.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (   0, uint256("0x00000a10f7ce671e773330376ce892a6c0b93fbc05553ebbf659b11e3bf9188d"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1400408750,
        0,
        100
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (fTestNet) return true; // Testnet has no checkpoints
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (fTestNet) return 0; // Testnet has no checkpoints
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (fTestNet) return NULL; // Testnet has no checkpoints
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

	uint256 GetLatestHardenedCheckpoint()
    {
        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;
        return (checkpoints.rbegin()->second);
    }
}
