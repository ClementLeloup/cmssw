#ifndef Input_RootDelayedReader_h
#define Input_RootDelayedReader_h

/*----------------------------------------------------------------------

RootDelayedReader.h // used by ROOT input sources

$Id: RootDelayedReader.h,v 1.1 2006/01/07 00:46:23 wmtan Exp $

----------------------------------------------------------------------*/

#include <memory>
#include <map>
#include <string>
#include "boost/shared_ptr.hpp"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/DelayedReader.h"
#include "IOPool/Input/src/Inputfwd.h"
#include "TBranch.h"

namespace edm {

  //------------------------------------------------------------
  // Class RootDelayedReader: pretends to support file reading.
  //

  class RootDelayedReader : public DelayedReader {
  public:
    typedef input::BranchMap BranchMap;
    typedef input::EntryNumber EntryNumber;
    RootDelayedReader(EntryNumber const& entry,
	 boost::shared_ptr<const BranchMap> bMap,
	 boost::shared_ptr<const TFile> filePtr);
    virtual ~RootDelayedReader();
  private:
    RootDelayedReader(RootDelayedReader const&); // disable copy construction
    RootDelayedReader & operator=(RootDelayedReader const&); // disable assignment
    virtual std::auto_ptr<EDProduct> get(BranchKey const& k, EDProductGetter const* ep) const;
    BranchMap const& branches() const {return *branches_;}
    EntryNumber const entryNumber_;
    boost::shared_ptr<const BranchMap> branches_;
    boost::shared_ptr<const TFile> filePtr_;
  }; // class RootDelayedReader
  //------------------------------------------------------------
}
#endif
