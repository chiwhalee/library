//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#ifndef __IQCOMBINER_H
#define __IQCOMBINER_H
#include "combiner.h"
#include "condenser.h"

/*
   Combine several indices into one index without loss of states.
   If the IQCombiner C is created from indices of IQTensor T, then
   an identity is   T * C * conj(C).  This looks like (for example)

        ___ __          __
       /      \        /
   ---T---- ---C-- --cC---
   where cC is conj(C).  Use of IQCombiners is efficient, whereas
   use of IQTensors for this purpose would not be.
*/
class IQCombiner
    {
    public:

    typedef std::vector<IQIndex>::const_iterator 
    left_it;

    IQCombiner();

    IQCombiner(
	    const IQIndex& l1, const IQIndex& l2 = IQIndex::Null(), 
        const IQIndex& l3 = IQIndex::Null(), const IQIndex& l4 = IQIndex::Null(), 
	    const IQIndex& l5 = IQIndex::Null(), const IQIndex& l6 = IQIndex::Null());

    bool 
    doCondense() const { return do_condense; }
    void 
    doCondense(bool val);

    void 
    reset();

    void 
    addleft(const IQIndex& l); 	// Include another left index

    const std::pair<left_it,left_it> 
    left() const 
        { 
        return std::make_pair(left_.begin(),left_.end()); 
        }

    inline bool 
    isInit() const { return initted; }

    // Initialize after all lefts are there and before being used
    void 
    init(std::string rname = "combined", IndexType type = Link, 
         Arrow dir = Switch, int primelevel = 0) const;
    
    operator IQTensor() const;

    const IQIndex& 
    right() const;

    int 
    findindex(const IQIndex& i) const;

    bool 
    hasindex(const IQIndex& I) const;

    bool 
    hasindex(const Index& I) const;

    int 
    num_left() const { return int(left_.size()); }

    void
    doprime(PrimeType pr, int inc = 1);

    friend IQCombiner
    primed(IQCombiner C, int inc = 1);

    void 
    conj();

    friend std::ostream& 
    operator<<(std::ostream & s, const IQCombiner & c);

    IQTensor 
    operator*(const IQTensor& t) const { IQTensor res; product(t,res); return res; }

    friend inline IQTensor 
    operator*(const IQTensor& t, const IQCombiner& c) { return c.operator*(t); }

    void 
    product(IQTensor t, IQTensor& res) const;

    private:

    /////////////
    //
    // Data Members
    //

    std::vector<IQIndex> left_;
    mutable IQIndex right_;
    mutable std::vector<Combiner> combs;
    mutable bool initted;

    mutable Condenser cond;
    mutable IQIndex ucright_;
    bool do_condense;

    //
    /////////////

    typedef std::map<ApproxReal, Combiner>::iterator
    setcomb_it;

    typedef std::map<Index, Combiner>::iterator
    rightcomb_it;

    };

class QCounter
    {
    public:

    QCounter(const std::vector<IQIndex>& v)
        : 
        v_(v),
        don(false),
        n_(v.size()),
        ind_(v.size(),0)
        {
        for(size_t j = 0; j < v_.size(); ++j)
            {
            n_[j] = v_[j].nindex();
            }
        }

    bool 
    notdone() const { return !don; }

    QCounter& 
    operator++()
        {
        int nn = n_.size();
        ind_[0]++;
        if(ind_[0] >= n_[0])
            {
            for(int j = 1; j < nn; j++)
                {
                ind_[j-1] = 0;
                ++ind_[j];
                if(ind_[j] < n_[j]) break;
                }
            }
        if(ind_[nn-1] >= n_[nn-1])
            {
            ind_ = std::vector<int>(nn,0);
            don = true;
            }

        return *this;
        }

    void 
    getVecInd(std::vector<Index>& vind, QN& q) const
        {
        vind.resize(v_.size());
        q = QN(); 
        for(size_t i = 0; i < ind_.size(); ++i)
            {
            const IQIndex& I = v_[i];
            int j = ind_[i]+1;
            vind[i] = I.index(j);
            q += I.qn(j)*I.dir();
            }
        }

    private:

    const std::vector<IQIndex>& v_;
    bool don;
    std::vector<int> n_, ind_;

    };

inline IQCombiner::
IQCombiner() 
    : initted(false), 
      do_condense(false) 
    { }

inline IQCombiner::
IQCombiner(
    const IQIndex& l1, const IQIndex& l2, 
    const IQIndex& l3, const IQIndex& l4, 
    const IQIndex& l5, const IQIndex& l6)
    : initted(false), 
      do_condense(false)
    {
    if(l1 == IQIndex::Null()) Error("Null IQIndex");
    if(l1 != IQIndex::Null()) left_.push_back(l1); 
    if(l2 != IQIndex::Null()) left_.push_back(l2);
    if(l3 != IQIndex::Null()) left_.push_back(l3); 
    if(l4 != IQIndex::Null()) left_.push_back(l4);
    if(l5 != IQIndex::Null()) left_.push_back(l5); 
    if(l6 != IQIndex::Null()) left_.push_back(l6);
    Foreach(IQIndex& L, left_) L.conj();
    }

inline
void IQCombiner::
doCondense(bool val)
    {
    if(initted) 
        Error("Can't set doCondense after already initted.");
    do_condense = val;
    }

inline 
void IQCombiner::
reset()
    {
    left_.clear();
    initted = false;
    }

inline
void IQCombiner::
addleft(const IQIndex& l) 	// Include another left index
	{ 
    if(l == IQIndex::Null()) Error("Null IQIndex");
    left_.push_back(l);
    //Flip arrows to make combiner compatible with
    //the IQTensor from which it got its left indices
    left_.back().conj();
    initted = false;
	}

inline
void IQCombiner::
init(std::string rname, IndexType type, 
     Arrow dir, int primelevel) const 
    {
    if(initted) return;
    if(left_.size() == 0)
        Error("No left indices in IQCombiner.");

    Arrow rdir; 
    if(dir == Switch) //determine automatically
        {
        rdir = Switch*left_.back().dir();

        //Prefer to derive right Arrow from Link indices
        for(size_t j = 0; j < left_.size(); ++j)
        if(left_[j].type() == Link) 
            { 
            rdir = Switch*left_[j].dir(); 
            break;
            }
        }
    else
        { rdir = dir; }

    //Construct individual Combiners
    QCounter c(left_);
    std::vector<inqn> iq;
    for( ; c.notdone(); ++c)
        {
        std::vector<Index> vind;
        QN q;
        c.getVecInd(vind, q); // updates vind and q
        q *= -rdir;

        combs.push_back(Combiner());
        Combiner& co = combs.back();
        co.addleft(vind);
        co.init(rname+q.toString(),type,rdir,primelevel);

        iq.push_back(inqn(co.right(),q));
        }
    if(do_condense) 
        {
        ucright_ = IQIndex(rname,iq,rdir,primelevel);
        std::string cname = "cond::" + rname;
        cond = Condenser(ucright_,cname);
        right_ = cond.smallind();
        }
    else 
        {
        right_ = IQIndex(rname,iq,rdir,primelevel);
        }
    initted = true;
	}

inline IQCombiner::
operator IQTensor() const
    {
    if(!initted) Error("IQCombiner::operator IQTensor(): IQCombiner not initialized.");

    std::vector<IQIndex> iqinds(left_);
    iqinds.push_back((do_condense ? ucright_ : right_));
    IQTensor res(iqinds);

    Foreach(const Combiner& co, combs)
        {
        //Here we are using the fact that Combiners
        //can be converted to ITensors
        res.insert(co);
        }

    //Combiners should always have the 
    //structure of zero divergence IQTensors
    DO_IF_DEBUG(checkQNs(res));

    if(do_condense) 
        { 
        IQTensor rcopy(res); 
        cond.product(rcopy,res); 
        }

    return res;
    }

inline
const IQIndex& IQCombiner::
right() const 
    { 
    init();
    return right_;
    }

inline
int IQCombiner::
findindex(const IQIndex& i) const
	{
    for(size_t j = 0; j < left_.size(); ++j)
    if(left_[j] == i) return j;
    return -1;
	}

inline
bool IQCombiner::
hasindex(const IQIndex& I) const
	{
    for(size_t j = 0; j < left_.size(); ++j)
        if(left_[j] == I) return true;
    return false;
	}

inline
bool IQCombiner::
hasindex(const Index& i) const
    {
    for(size_t j = 0; j < left_.size(); ++j)
        if(left_[j].hasindex(i)) return true;
    return false;
    }

void inline IQCombiner::
doprime(PrimeType pr, int inc)
    {
    Foreach(IQIndex& ll, left_)
        ll.doprime(pr,inc);
    Foreach(Combiner& co, combs)
        co.doprime(pr,inc);
    if(initted)
        {
        right_.doprime(pr,inc);
        if(do_condense) 
            {
            cond.doprime(pr,inc);
            ucright_.doprime(pr,inc);
            }
        }
    }

IQCombiner inline
primed(IQCombiner C, int inc)
    {
    C.doprime(primeBoth,inc);
    return C;
    }


inline
void IQCombiner::
conj() 
    { 
    init();
    Foreach(IQIndex& I, left_) I.conj(); 
    if(do_condense) 
        {
        cond.conj();
        ucright_.conj();
        }
    right_.conj();
    }

inline 
std::ostream& 
operator<<(std::ostream & s, const IQCombiner & c)
    {
    if(c.isInit())
        { s << std::endl << "Right index is " << c.right() << "\n"; }
    else
        { s << std::endl << "Right index is not initialized\n\n"; }
    s << "Left indices: \n";
    Foreach(const IQIndex& I, c.left_) s << I << std::endl;
    return s << "\n\n";
    }

inline
void IQCombiner::
product(IQTensor T, IQTensor& res) const
    {
    init();
    std::vector<IQIndex> iqinds;

    int j;
    if((j = T.findindex(right_)) != 0)
        {
        //
        //T has right IQIndex, expand it
        //

        IQTensor T_uncondensed;
        if(do_condense) 
            { 
            cond.product(T,T_uncondensed); 
            j = T_uncondensed.findindex(ucright_);
            }
        const IQTensor& T_ = (do_condense ? T_uncondensed : T);
        const IQIndex& r = (do_condense ? ucright_ : right_);

        if(Global::checkArrows())
            if(T_.index(j).dir() == r.dir())
                {
                std::cerr << "IQTensor = " << T_ << std::endl;
                std::cerr << "IQCombiner = " << *this << std::endl;
                std::cerr << "IQIndex from IQTensor = " << T_.index(j) << std::endl;
                std::cerr << "(Right) IQIndex from IQCombiner = " << r << std::endl;
                Error("Incompatible arrow directions in operator*(IQTensor,IQCombiner).");
                }
        copy(T_.const_iqind_begin(),T_.const_iqind_begin()+j-1,std::back_inserter(iqinds));
        copy(left_.begin(),left_.end(),std::back_inserter(iqinds));
        copy(T_.const_iqind_begin()+j,T_.const_iqind_end(),std::back_inserter(iqinds));

        res = IQTensor(iqinds);

        std::map<Index, const Combiner*> rightcomb;
        Foreach(const Combiner& co, combs)
            {
            rightcomb[co.right()] = &co;
            }

        Foreach(const ITensor& tt, T_.itensors())
            {
            for(int k = 1; k <= tt.r(); ++k)
                {
                if(r.hasindex(tt.index(k)))
                    { 
                    res += (*(rightcomb[tt.index(k)]) * tt); 
                    break;
                    }
                } //end for
            } //end Foreach

        }
    else
        {
        //
        //T has left IQIndex's, combine them
        //

        //res will have all IQIndex's of T not in the left of c
        Foreach(const IQIndex& I, T.iqinds()) 
            { 
            if(!hasindex(I)) iqinds.push_back(I); 
            }
        //and res will have c's right IQIndex
        if(do_condense) iqinds.push_back(ucright_);
        else            iqinds.push_back(right_);

        res = IQTensor(iqinds);

        //Check left indices
        Foreach(const IQIndex& I, left_)
            {
            if((j = T.findindex(I)) == 0)
                {
                std::cerr << "Could not find left IQIndex " << I << "\n";
                T.printIndices("T");
                std::cerr << "Left indices\n";
                for(size_t j = 0; j < left_.size(); ++j)
                    { 
                    std::cerr << j SP left_[j] << "\n"; 
                    }
                Error("bad IQCombiner IQTensor product");
                }
            else //IQIndex is in left
                {
                //Check arrow directions
                if(Global::checkArrows())
                    if(T.index(j).dir() == I.dir())
                        {
                        PrintIndices(T);
                        Print((*this));
                        std::cerr << "IQIndex from IQTensor = " << T.index(j) << std::endl;
                        std::cerr << "(Left) IQIndex from IQCombiner = " << I << std::endl;
                        Error("Incompatible arrow directions in operator*(IQTensor,IQCombiner).");
                        }
                }
            }

        //Create map of Combiners using uniqueReal as key
        std::map<ApproxReal, const Combiner*> combmap;
        Foreach(const Combiner& co, combs)
            {
            combmap[co.uniqueReal()] = &co;
            }

        //Loop over each block in T and apply appropriate
        //Combiner (determined by the uniqueReal of the 
        //combined Indices)
        Foreach(const ITensor& t, T.blocks())
            {
            Real block_ur = 0;
            for(int k = 1; k <= t.r(); ++k)
                {
                if(this->hasindex(t.index(k))) 
                    block_ur += t.index(k).uniqueReal();
                }

            if(combmap.count(block_ur) == 0)
                {
                Print(t);
                std::cerr << "\nleft indices \n";
                for(size_t j = 0; j < left_.size(); ++j)
                    { std::cerr << j << " " << left_[j] << "\n"; }
                std::cerr << "\n\n";

                typedef std::map<ApproxReal, const Combiner*>::const_iterator
                combmap_const_it;
                for(combmap_const_it uu = combmap.begin();
                    uu != combmap.end(); ++uu)
                    {
                    std::cout << "Combiner: " << std::endl;
                    std::cout << *(uu->second) << std::endl;
                    }
                Error("no combmap entry for block_ur in IQCombiner prod");
                }

            res += (*combmap[block_ur] * t);
            }

        if(do_condense) 
            { 
            IQTensor rcopy(res); 
            cond.product(rcopy,res); 
            }
        }
    } //void product(const IQTensor& T, IQTensor& res) const


#endif
