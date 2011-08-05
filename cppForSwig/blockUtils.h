#ifndef _BLOCKUTILS_H_
#define _BLOCKUTILS_H_

#include <stdio.h>
#include <iostream>
#ifdef WIN32
   #include <cstdint>
#else
   #include <stdlib.h>
   #include <inttypes.h>
   #include <cstring>
#endif
#include <fstream>
#include <vector>
#include <queue>
#include <list>
#include <map>
#include <limits>

#include "binaryData.h"

#include "cryptlib.h"
#include "sha.h"


#define HEADER_SIZE       80
#define HEADERS_PER_CHUNK 25000
#define BHM_CHUNK_SIZE    (HEADER_SIZE*HEADERS_PER_CHUNK)



//#ifdef MAIN_NETWORK
   #define MAGICBYTES "f9beb4d9"
   #define CURR_APPROX_NUM_BLOCKS 130000
   #define GENESIS_HASH_HEX "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000"
//#else
   //#define MAGICBYTES "fabfb5da"
   //#define GENESIS_HASH_HEX "08b067b31dc139ee8e7a76a4f2cfcca477c4c06e1ef89f4ae308951907000000"
//#endif



using namespace std;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// This class doesn't hold actually block header data, it only holds pointers
// to where the data is in the BlockHeaderManager.  So there is a single place
// where all block headers are stored, and this class tells us where exactly
// is the one we want.
class BlockHeaderRef
{

   friend class BlockHeadersManager;

public: 

   uint32_t       getVersion(void)    { return *version_Raw_;                   }
   binaryData     getPrevHash(void)   { return  binaryData(prevHash_Raw_, 
                                                           prevHash_Raw_+32);   }
   binaryData     getMerkleRoot(void) { return  binaryData(merkleRoot_Raw_, 
                                                           merkleRoot_Raw_+32); }
   uint32_t       getTimestamp(void)  { return *timestamp_Raw_;                 }
   binaryData     getDiffBits(void)   { return  binaryData(diffBits_Raw_, 
                                                           diffBits_Raw_+4);    }
   uint32_t       getNonce(void)      { return *nonce_Raw_;                     }

   binaryData     getThisHash(void)   { return  thisHash_;                      }
   binaryData     getNextHash(void)   { return  nextHash_;                      }
   uint32_t       getNumTx(void)      { return  numTx_;                         }
   
   void setVersion(uint32_t i)        { *version_Raw_ = i;                      }
   void setPrevHash(binaryData str)   { str.copyTo(prevHash_Raw_, 32);          }
   void setMerkleRoot(binaryData str) { str.copyTo(merkleRoot_Raw_, 32);        }
   void setTimestamp(uint32_t i)      { *timestamp_Raw_ = i;                    }
   void setDiffBits(binaryData str)   { str.copyTo(diffBits_Raw_, 4);           }
   void setNonce(uint32_t i)          { *nonce_Raw_ = i;                        }
   void setNextHash(binaryData str)   { nextHash_.copyFrom(str);                }

   //binaryData serialize(void) 
   //{ 
      //binaryData bdout(HEADER_SIZE);
      //memcpy(&(bdout[0]), blockStart_, HEADER_SIZE);
      //return binaryData;
   //}

   void  unserialize(binaryData strIn) 
   { 
      assert(blockStart_==NULL);
      memcpy(blockStart_, strIn.getConstPtr(), HEADER_SIZE);
   }

   bool isInvalid(void) { return (blockStart_==NULL); }

   BlockHeaderRef(uint8_t*    blkptr  = NULL, 
                  binaryData* thisHash = NULL,
                  uint64_t    fileLoc = numeric_limits<uint64_t>::max()) :
      blockStart_(blkptr),
      prevHash_(32),
      thisHash_(32),
      nextHash_(32),
      numTx_(-1),
      fileByteLoc_(fileLoc),  
      difficultyFlt_(-1.0),
      difficultySum_(-1.0),
      blockHeight_(0),
      isMainBranch_(false),
      isOrphan_(false),
      isFinishedCalc_(false)
   {
      if(blkptr != NULL)
      {
         version_Raw_    = (uint32_t*)(blockStart_ +  0);
         prevHash_Raw_   =            (blockStart_ +  4);
         merkleRoot_Raw_ =            (blockStart_ + 36);
         timestamp_Raw_  = (uint32_t*)(blockStart_ + 68);
         diffBits_Raw_   =            (blockStart_ + 72);
         nonce_Raw_      = (uint32_t*)(blockStart_ + 76);

         if(! (thisHash == NULL) )
            thisHash_.copyFrom(*thisHash);

         prevHash_ = getPrevHash();

      }
   }

   static double convertDiffBitsToDouble(uint32_t diffBits)
   {
       int nShift = (diffBits >> 24) & 0xff;
       double dDiff = (double)0x0000ffff / (double)(diffBits & 0x00ffffff);
   
       while (nShift < 29)
       {
           dDiff *= 256.0;
           nShift++;
       }
       while (nShift > 29)
       {
           dDiff /= 256.0;
           nShift--;
       }
   
       return dDiff;
   }

   void printBlockHeader(ostream & os=cout)
   {
      os << "Block Information: " << blockHeight_ << endl;
      os << "\tHeight:       " << blockHeight_ << endl;
      os << "\tHash:         " << thisHash_.toHex().c_str() << endl;
      os << "\tTimestamp:    " << getTimestamp() << endl;
      os << "\tPrev Hash:    " << prevHash_.toHex().c_str() << endl;
      os << "\tMerkle Root:  " << getMerkleRoot().toHex().c_str() << endl;
      os << "\tDifficulty:   " << difficultyFlt_ 
                             << "(" << getDiffBits().toHex().c_str() << ")" << endl;
      os << "\tCumulDiff:    " << difficultySum_ << endl;
      os << "\tNonce:        " << getNonce() << endl;
      os << "\tFile Offset:  " << fileByteLoc_ << endl;
   }


private:
   uint8_t* blockStart_;

   // All these pointers point to data managed by another class.
   // As such, it is unnecessary to deal with any memory mgmt. 
   uint32_t*  version_Raw_;
   uint8_t*   prevHash_Raw_;
   uint8_t*   merkleRoot_Raw_;
   uint32_t*  timestamp_Raw_;
   uint8_t*   diffBits_Raw_; 
   uint32_t*  nonce_Raw_; 

   // Some more data types to be stored with the header, but not
   // part of the official serialized header data, so these are
   // actual members of the BlockHeaderRef.
   binaryData     prevHash_;
   binaryData     thisHash_;
   binaryData     nextHash_;
   uint32_t       numTx_;
   uint32_t       blockHeight_;
   uint64_t       fileByteLoc_;
   double         difficultyFlt_;
   double         difficultySum_;
   bool           isMainBranch_;
   bool           isOrphan_;
   bool           isFinishedCalc_;

   // We should keep the genesis hash handy 
   static binaryData GenesisHash_;
   static binaryData EmptyHash_;
};



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// The goal of this class is to create a memory pool in RAM that looks exactly
// the same as the block-headers storage on disk.  There is no serialization
// or unserialization, we just copy back and forth between disk and RAM, and 
// we're done.  So it should be about as fast as theoretically possible, you 
// are limited only by your disk I/O speed.
//
// This is more of a simple test, which will later be applied to the entire
// blockchain.  If it works as expected, then this will potentially be useful
// for the official BTC client which seems to have some speed problems at 
// startup and shutdown.
//
// This class is a singleton -- there can only ever be one, accessed through
// the static method GetInstance().  This method gets the single instantiation
// of the BHM class, and then its public members can be used to access the 
// block data that is sitting in memory.
//
class BlockHeadersManager
{
private:
   // We will store headers in chunks of 10,000 (800,000 bytes)
   // We will maintain a std::list of such chunks, so that we can
   // efficiently add more chunks as the number of headers increases
   //
   // Note this is a linked list, but we simulatneously maintain a 
   // list of pointers to these chunks so we can also access by
   // index (copying a vector of pointers to expand it is a lot better
   // than copying a vector of 1MB chunks)
   list<binaryData>                                   chunks_;
   vector<list<binaryData>::iterator>                 chunkPtrs_;
   vector<uint8_t*>                                   rawHeaderPtrs_;

   map<binaryData, BlockHeaderRef>                    headerMap_;
   vector<map<binaryData, BlockHeaderRef>::iterator>  headersByHeight_;
   queue<uint8_t*>                                    deletedPtrs_;
   uint32_t                                           nextHeaderIndex_;  
   BlockHeaderRef*                                    topBlockPtr_;

   static BlockHeadersManager *                       theOnlyBHM_;
   static CryptoPP::SHA256                            sha256_;


   // Member descriptions:
   //
   //    chunks_    -  A list of binary data chunks to hold the headers.
   //                  Chunk size could be something like 10,000 headers.
   //                  The goal is to hold the headers in large chunks
   //                  of contiguous memory so that file I/O is faster.
   //    chunkPtrs_ -  chunks_ is a linked-list, so we actively maintain
   //                  pointers to each of it's elements so we can do
   //                  random access.  Since elements of chunks_ are 
   //                  >500 KB, it's much cheaper to have to copy a list
   //                  of pointers upon vector expansion, thatn to copy
   //                  the entire array of data itself.  
   //
   //    rawHeaderPtrs_ - contains pointers to the original header information
   //                  in the order they were read from the blockfile.  This
   //                  will be most useful for operations that require looping
   //                  over the entire set of block headers in the memory pool
   //                  in arbitrary order
   //
   //    BlockHeaderRef -  contains pointers to the official block header
   //                  information within chunks_ structure.  Also includes
   //                  some extra (non-pointer) data such as it's own hash 
   //                  value, and location of the TX data in the block file.
   //
   //    headerMap_ -  Map<BinaryData, BlockHeaderRef> to map the block 
   //                  header hashes to the headers in the chunks_ structure.
   //
   //    headersByHeight_ - will eventually be a lookup table for block 
   //                  headers based on their height relative to the genesis
   //                  block.  If a block is not in the main chain, it won't
   //                  be in this vector
   //                  
   //    deletedPtrs_  - (not implemented) if we ever decide to delete headers
   //                  from the memory pool, we'll be left with gaps in the
   //                  chunks_ structures.  We would then want to back-fill
   //                  these gaps with new header data.  The idea is to add
   //                  a pointer to the location of the deleted header, and
   //                  check this queue for such gaps before adding a new 
   //                  header to the memory pool.  
   //
   //    theOnlyBHM_  - This is a "singleton class" which means there will 
   //                  only ever be one.  This is a ptr to that instantiation.
   //
   //    sha256_    -  The CryptoPP object to be used for SHA256 hashing.
   //                  (probably unnecessary)
   //
   //              

private:
   // Set the constructor to private so that only one can ever be created
   BlockHeadersManager(void) : 
         chunks_(0), 
         chunkPtrs_(0), 
         nextHeaderIndex_(0),
         topBlockPtr_(NULL)
   {
      addChunk();
      headerMap_.clear();
   }

public:

   /////////////////////////////////////////////////////////////////////////////
   // The only way to "create" a BHM is with this method, which creates it
   // if one doesn't exist yet, or returns a reference to the only one
   // that will ever exist
   static BlockHeadersManager & GetInstance(void) 
   {
      static bool bhmCreatedYet_ = false;
      if( !bhmCreatedYet_ )
      {
         theOnlyBHM_ = new BlockHeadersManager;
         bhmCreatedYet_ = true;
      }
      return (*theOnlyBHM_);
   }

   /////////////////////////////////////////////////////////////////////////////
   BlockHeaderRef getTopBlock(void)
   {
      return *topBlockPtr_;
   }

   /////////////////////////////////////////////////////////////////////////////
   BlockHeaderRef getGenesisBlock(void)
   {
      return headerMap_[BlockHeaderRef::GenesisHash_];
   }

   /////////////////////////////////////////////////////////////////////////////
   // Get a blockheader based on its height on the main chain
   BlockHeaderRef getHeaderByHeight(int index)
   {
      if( index>=0 && index<(int)headersByHeight_.size())
         return headersByHeight_[index]->second;
   }

   /////////////////////////////////////////////////////////////////////////////
   // The most common access method is to get a block by its hash
   BlockHeaderRef getHeaderByHash(binaryData const & blkHash)
   {
      map<binaryData, BlockHeaderRef>::iterator it = headerMap_.find(blkHash);
      if(it==headerMap_.end())
         return BlockHeaderRef(NULL);
      else
         return headerMap_[blkHash];
   }


   /////////////////////////////////////////////////////////////////////////////
   // Add headers from a file that is serialized identically to the way
   // we have laid it out in memory.  Return the number of bytes read
   uint64_t importHeadersFromHeaderFile(std::string filename)
   {
      cout << "Reading block headers from file: " << filename.c_str() << endl;
      ifstream is(filename.c_str(), ios::in | ios::binary);
      is.seekg(0, ios::end);
      size_t filesize = (size_t)is.tellg();
      is.seekg(0, ios::beg);
      cout << filename.c_str() << " is " << filesize << " bytes" << endl;
      if((unsigned int)filesize % HEADER_SIZE != 0)
      {
         cout << "filesize=" << filesize << " is not a multiple of header size!" << endl;
         return -1;
      }
      
      // We'll need this information to do the indexing, later
      int numHeadersBefore = nextHeaderIndex_;
      int numHeadersToAdd  = filesize / HEADER_SIZE;
      int numHeadersTotal  = numHeadersBefore+numHeadersToAdd;

      allocate(numHeadersBefore + numHeadersToAdd);
      int numHeadersLeft = numHeadersToAdd;
      int numToCopy = nHeadersLeftInChunk();  // the first copy
      while(numToCopy>0) 
      {
         cout << "\tCopying " << numHeadersLeft << " headers" << endl;
         int nBytes = numToCopy * HEADER_SIZE;
         uint8_t* nextCopyPtr = getNextEmptyPtr();
         is.read( (char *)nextCopyPtr, nBytes);
         incrementHeaderIndex(numToCopy);
         
         numHeadersLeft -= numToCopy;
         numToCopy = min(numHeadersLeft, HEADERS_PER_CHUNK);
      }

      cout << "Done with everything!  " << filesize << " bytes read!" << endl;
      return filesize;      

   }

   /////////////////////////////////////////////////////////////////////////////
   void indexHeaders(int headerIdx0=0, int headerIdx1=0)
   {
      if(headerIdx1 <= headerIdx0)
         headerIdx1 = nextHeaderIndex_;

      cout << "Done reading headers, now indexing the new data." << endl;
      // Now with all the data in memory, create BlockHeaderRef objects
      rawHeaderPtrs_.resize(nextHeaderIndex_);
      binaryData theHash(32);
      for( int i=headerIdx0; i<headerIdx1; i++)
      {
         // we'll come back to this
         int chunkIndex  = i / HEADERS_PER_CHUNK;
         int headerIndex = i % HEADERS_PER_CHUNK;
         binaryData & chnkptr = *(chunkPtrs_[chunkIndex]);
         uint8_t* thisHeaderPtr = chnkptr.getPtr() + (HEADER_SIZE * headerIndex);
         rawHeaderPtrs_[i] = thisHeaderPtr;

         getHash( thisHeaderPtr, theHash);
         //cout << theHash.toHex().c_str() << "  " << i << endl;
         headerMap_[theHash] = BlockHeaderRef(thisHeaderPtr);
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   uint32_t importHeadersFromBlockFile(std::string filename)
   {
      binaryStreamBuffer bsb(filename, 25*1024*1024);  // use 25 MB buffer
      
      bool readMagic  = false;
      bool readVarInt = false;
      bool readBlock  = false;
      uint32_t numBlockBytes;

      binaryData magicBucket(4);
      binaryData magicStr(4);
      magicStr.createFromHex(MAGICBYTES);
      binaryData hashBucket(32);

      // The resize value is only approximate, and not at all necessary 
      // to get it right.  I'm just avoiding the growing pains of starting
      // with an empty list, and pushing 140,00 blocks.  The underlying
      // vector would have to expand a couple times, doing an expensive
      // full copy every time
      rawHeaderPtrs_.resize(CURR_APPROX_NUM_BLOCKS);

      // While there is still data left in the stream (file), pull it
      while(bsb.streamPull())
      {
         // Data has been pulled into the buffer, process all of it
         while(bsb.getBufferSizeRemaining() > 1)
         {
            static int i = 0;
            //cout << "Block# " << i++ << ":";
            // The first four bytes are always the magic bytes
            if( !readMagic )
            {
               if(bsb.getBufferSizeRemaining() < 4)
                  break;
               bsb.reader().get_binaryData(magicBucket, 4);
               if( !(magicBucket == magicStr) )
               {
                  //cerr << "Magic string does not match network!" << endl;
                  //cerr << "\tExpected: " << MAGICBYTES << endl;
                  //cerr << "\tReceived: " << magicBucket.toHex() << endl;
                  break;
               }
               readMagic = true;
            }

            // If we haven't read the blockdata-size yet, do it
            if( !readVarInt )
            {
               // TODO:  Whoops, this isn't a VAR_INT, just a 4-byte num
               if(bsb.getBufferSizeRemaining() < 4)
                  break;
               numBlockBytes = bsb.reader().get_uint32_t();
               readVarInt = true;
            }


            // If we haven't read the header yet, do it
            uint8_t* thisBlockPtr  = getNextEmptyPtr();
            uint64_t blkByteOffset = bsb.getFileByteLocation();
            if( !readBlock )
            {
               if(bsb.getBufferSizeRemaining() < numBlockBytes)
                  break;

               bsb.reader().get_binaryData(thisBlockPtr, HEADER_SIZE);
               incrementHeaderIndex();

               // Here we are only reading headers, so we advance past txData
               bsb.reader().advance((uint32_t)(numBlockBytes-HEADER_SIZE));
               readBlock = true;
            }

            
            readMagic  = false;
            readVarInt = false;
            readBlock  = false;

            // The header has been added to the memory pool, but not indexed
            // in a way that we can locate it efficiently.
            rawHeaderPtrs_.push_back(thisBlockPtr);
            getHash(thisBlockPtr, hashBucket);
            //cout << hashBucket.toHex().c_str() << endl;
            headerMap_[hashBucket] = BlockHeaderRef(thisBlockPtr, 
                                                    &hashBucket,
                                                    blkByteOffset+HEADER_SIZE);
         }
      }

      return nextHeaderIndex_;
   }


   /////////////////////////////////////////////////////////////////////////////
   // Not sure exactly when this would get used...
   void addHeader(binaryData const & binHeader)
   {
      uint8_t* newHeaderLoc = getNextEmptyPtr();
      rawHeaderPtrs_.push_back(newHeaderLoc);
      binHeader.copyTo(newHeaderLoc, HEADER_SIZE);
      incrementHeaderIndex();
   }


   /////////////////////////////////////////////////////////////////////////////
   int nHeadersLeftInChunk(void)
   {
      return (HEADERS_PER_CHUNK - (nextHeaderIndex_ % HEADERS_PER_CHUNK));
   }


   /////////////////////////////////////////////////////////////////////////////
   void defrag(void)
   {
      // TODO:  Still need to write this.  Will move the headers at the
      //        back of the list into spots previously occupied by other
      //        block that were deleted.
      //          
   }
   

   /////////////////////////////////////////////////////////////////////////////
   static void getHash(uint8_t* blockStart, binaryData & hashOut)
   {
      if(hashOut.getSize() != 32)
         hashOut.resize(32);
      sha256_.CalculateDigest(hashOut.getPtr(), blockStart, HEADER_SIZE);
      sha256_.CalculateDigest(hashOut.getPtr(), hashOut.getPtr(), 32);
   }

   // This returns false if our new main branch does not include the previous
   // topBlock.  If this returns false, that probably means that we have
   // previously considered some blocks to be valid that no longer are valid.
   bool organizeChain(void)
   {
      // Set genesis block
      BlockHeaderRef & genBlock = headerMap_[BlockHeaderRef::GenesisHash_];
      genBlock.blockHeight_    = 0;
      genBlock.difficultyFlt_  = 1.0;
      genBlock.difficultySum_  = 1.0;
      genBlock.isMainBranch_   = true;
      genBlock.isOrphan_       = false;
      genBlock.isFinishedCalc_ = true;

      binaryData const & GenesisHash_ = BlockHeaderRef::GenesisHash_;
      binaryData const & EmptyHash_   = BlockHeaderRef::EmptyHash_;
      if(topBlockPtr_ == NULL)
         topBlockPtr_ = &(headerMap_[BlockHeaderRef::GenesisHash_]);

      // Store the old top block so we can later check whether it is included 
      // in the new chain organization
      BlockHeaderRef* prevTopBlockPtr = topBlockPtr_;

      // Iterate over all blocks, track the maximum difficulty-sum block
      map<binaryData, BlockHeaderRef>::iterator iter;
      uint32_t maxBlockHeight = 0;
      double   maxDiffSum = 0;
      for( iter  = headerMap_.begin(); iter != headerMap_.end(); iter ++)
      {
         // *** The magic happens here
         double thisDiffSum = traceChainDown(iter->second);
         // ***
         
         if(thisDiffSum > maxDiffSum)
         {
            maxDiffSum     = thisDiffSum;
            topBlockPtr_   = &(iter->second);
         }
      }

      // Walk down the list one more time, set nextHash fields
      // Also set headersByHeight_;
      topBlockPtr_->nextHash_ = EmptyHash_;
      BlockHeaderRef* thisBlockPtr = topBlockPtr_;
      bool prevChainStillValid = (thisBlockPtr == prevTopBlockPtr);
      while( !thisBlockPtr->isFinishedCalc_ )
      {
         thisBlockPtr->isFinishedCalc_ = true;
         thisBlockPtr->isMainBranch_   = true;

         binaryData & childHash        = thisBlockPtr->thisHash_;
         thisBlockPtr                  = &(headerMap_[thisBlockPtr->prevHash_]);
         thisBlockPtr->nextHash_       = childHash;

         if(thisBlockPtr == prevTopBlockPtr)
            prevChainStillValid = true;
      }

      return prevChainStillValid;
   }

private:

   /////////////////////////////////////////////////////////////////////////////
   // Add another chunk of 10,000 block headers to the global memory pool
   void addChunk(void)
   {
      chunks_.push_back(binaryData(BHM_CHUNK_SIZE));
      list<binaryData>::iterator iter = chunks_.end();
      iter--;
      chunkPtrs_.push_back(iter);
   }

   /////////////////////////////////////////////////////////////////////////////
   // Bulk-allocate some space for a certain number of headers
   void allocate(int nHeaders)
   {
      int prevNChunk = (int)chunks_.size();
      int needNChunk = (nHeaders / HEADERS_PER_CHUNK) + 1;
      for(int i=prevNChunk+1; i<=needNChunk; i++)
         addChunk();
   }


   /////////////////////////////////////////////////////////////////////////////
   uint8_t* getNextEmptyPtr(void) const
   { 
      int chunkIndex  = nextHeaderIndex_ / HEADERS_PER_CHUNK;
      int headerIndex = nextHeaderIndex_ % HEADERS_PER_CHUNK;
      binaryData & chnkptr = *(chunkPtrs_[chunkIndex]);
      return chnkptr.getPtr() + (HEADER_SIZE * headerIndex);
   }

   /////////////////////////////////////////////////////////////////////////////
   bool incrementHeaderIndex(int nIncr=1)
   {
      int oldChunkIndex = nextHeaderIndex_ / HEADERS_PER_CHUNK;
      nextHeaderIndex_ += nIncr;
      int newChunkIndex = nextHeaderIndex_ / HEADERS_PER_CHUNK;
      if(newChunkIndex >= (int)chunks_.size())
         allocate(nextHeaderIndex_);
      // We return a indicator that we are in a different chunk than
      // we started.  This may 
      if(oldChunkIndex != newChunkIndex)
         return true;
      return false;
   }


   /////////////////////////////////////////////////////////////////////////////
   // Start from a node, trace down to the highest solved block, accumulate
   // difficulties and difficultySum values.  Return the difficultySum of 
   // this block.
   double traceChainDown(BlockHeaderRef & bhpStart)
   {
      if(bhpStart.difficultySum_ > 0)
         return bhpStart.difficultySum_;

      // Prepare some data structures for walking down the chain
      vector<double>          difficultyStack(nextHeaderIndex_);
      vector<BlockHeaderRef*>     bhpPtrStack(nextHeaderIndex_);
      uint32_t blkIdx = 0;
      double thisDiff;

      // Walk down the chain of prevHash_ values, until we find a block
      // that has a definitive difficultySum value (i.e. >0). 
      BlockHeaderRef* thisPtr = &bhpStart;
      map<binaryData, BlockHeaderRef>::iterator iter;
      bool isOrphanChain = false;
      while( thisPtr->difficultySum_ < 0)
      {
         thisDiff = BlockHeaderRef::convertDiffBitsToDouble(
                              *(uint32_t*)(thisPtr->diffBits_Raw_));
         difficultyStack[blkIdx] = thisDiff;
         bhpPtrStack[blkIdx]     = thisPtr;
         blkIdx++;


         iter = headerMap_.find(thisPtr->prevHash_);
         if( ! (iter == headerMap_.end()) )
            thisPtr = &(iter->second);
         else
         {
            // We didn't hit a known block, but we don't have this block's
            // ancestor in the memory pool, so we can't go back any further.
            iter->second.isOrphan_ = true;
            isOrphanChain = true;
            break;
         }
      }

      // If this is an orphan chain, then label all the blocks in this chain
      // as orphans and get out
      if(isOrphanChain)
      {
         for(uint32_t i=blkIdx-1; i>=0; i--)
         {
            bhpPtrStack[i]->isOrphan_ = true;
            return 0.0;
         }
      }

      // Now we have a stack of difficulties and pointers.  Walk back up
      // (by pointer) and accumulate the difficulty values 
      double   seedDiffSum = thisPtr->difficultySum_;
      uint32_t blkHeight   = thisPtr->blockHeight_;
      for(int32_t i=blkIdx-1; i>=0; i--)
      {
         seedDiffSum += difficultyStack[i];
         blkHeight++;
         thisPtr                 = bhpPtrStack[i];
         thisPtr->difficultyFlt_ = difficultyStack[i];
         thisPtr->difficultySum_ = seedDiffSum;
         thisPtr->blockHeight_   = blkHeight;
      }
      
      // Finally, we have all the difficulty sums calculated, return this one
      return bhpStart.difficultySum_;
     
   }



   
};



#endif