// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Manta
// Copyright (c) 2013-2014 Illumina, Inc.
//
// This software is provided under the terms and conditions of the
// Illumina Open Source Software License 1.
//
// You should have received a copy of the Illumina Open Source
// Software License 1 along with this program. If not, see
// <https://github.com/sequencing/licenses/>
//

#include <iostream>


template <typename ScoreType>
std::ostream&
operator<<(std::ostream& os, AlignmentResult<ScoreType>& alignment)
{
    os << "AlignerResult: score: " << alignment.score << "\n"
       << "\talignment: " << alignment.align << "\n";
    return os;
}



// traceback:
template <typename ScoreType>
template <typename SymIter, typename MatrixType>
void
SingleRefAlignerBase<ScoreType>::
backTraceAlignment(
    const SymIter queryBegin, const SymIter queryEnd,
    const SymIter refBegin, const SymIter refEnd,
    const size_t querySize, const size_t refSize,
    const MatrixType& ptrMatrix,
    const BackTrace<ScoreType>& btraceInput,
    AlignmentResult<ScoreType>& result) const
{
    BackTrace<ScoreType> btrace(btraceInput);

    assert(btrace.isInit);
    assert(btrace.refBegin <= refSize);
    assert(btrace.queryBegin <= querySize);

    result.score = btrace.max;

    // traceback:
    ALIGNPATH::path_t& apath(result.align.apath);
    ALIGNPATH::path_segment ps;

    // add any trailing soft-clip if we go off the end of the reference:
    if (btrace.queryBegin < querySize)
    {
        ps.type = ALIGNPATH::SOFT_CLIP;
        ps.length = (querySize-btrace.queryBegin);
    }

    while ((btrace.queryBegin>0) && (btrace.refBegin>0))
    {
        const AlignState::index_t nextMatrix(static_cast<AlignState::index_t>(ptrMatrix.val(btrace.queryBegin,btrace.refBegin).get(btrace.state)));

        if (btrace.state==AlignState::MATCH)
        {
            AlignerUtil::updatePath(apath,ps,ALIGNPATH::MATCH);
            btrace.queryBegin--;
            btrace.refBegin--;
        }
        else if ((btrace.state==AlignState::DELETE) || (btrace.state==AlignState::JUMP))
        {
            AlignerUtil::updatePath(apath,ps,ALIGNPATH::DELETE);
            btrace.refBegin--;
        }
        else if ((btrace.state==AlignState::INSERT) || (btrace.state==AlignState::JUMPINS))
        {
            AlignerUtil::updatePath(apath,ps,ALIGNPATH::INSERT);
            btrace.queryBegin--;
        }
        else
        {
            assert(false && "Unknown align state");
        }
        btrace.state=nextMatrix;
        ps.length++;
    }

    if (ps.type != ALIGNPATH::NONE) apath.push_back(ps);

    // soft-clip beginning of read if we fall off the end of the reference
    if (btrace.queryBegin!=0)
    {
        ps.type = ALIGNPATH::SOFT_CLIP;
        ps.length = btrace.queryBegin;
        apath.push_back(ps);
    }

    result.align.beginPos = btrace.refBegin;
    std::reverse(apath.begin(),apath.end());

    // if true, output final cigars using seq match '=' and mismatch 'X' symbols:
    static const bool isOutputSeqMatch(true);

    if (isOutputSeqMatch)
    {
        apath_add_seqmatch(queryBegin, queryEnd, (refBegin+result.align.beginPos), refEnd, apath);
    }

}
