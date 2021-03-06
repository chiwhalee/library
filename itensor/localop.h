//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_LOCAL_OP
#define __ITENSOR_LOCAL_OP
#include "iqtensor.h"

//
// The LocalOp class represents
// an MPO or other operator that
// has been projected into the
// reduced Hilbert space of 
// two sites of an MPS.
//
//   .-              -.
//   |    |      |    |
//   L - Op1 -- Op2 - R
//   |    |      |    |
//   '-              -'
//
// (Note that L, Op1, Op2 and R
//  are not required to have this
//  precise structure. L and R
//  can even be null in which case
//  they will not be used.)
//


template <class Tensor>
class LocalOp
    {
    public:

    typedef typename Tensor::IndexT
    IndexT;

    typedef typename Tensor::CombinerT
    CombinerT;

    //
    // Constructors
    //

    LocalOp();

    LocalOp(const Tensor& Op1, const Tensor& Op2, 
            const Tensor& L, const Tensor& R);

    //
    // Sparse Matrix Methods
    //

    void
    product(const Tensor& phi, Tensor& phip) const;

    Real
    expect(const Tensor& phi) const;

    Tensor
    deltaRho(const Tensor& rho, 
             const CombinerT& comb, Direction dir) const;

    Tensor
    deltaPhi(const Tensor& phi) const;

    void
    diag(Tensor& D) const;

    int
    size() const;

    //
    // Accessor Methods
    //

    void
    update(const Tensor& Op1, const Tensor& Op2, 
           const Tensor& L, const Tensor& R);

    const Tensor&
    L() const 
        { 
        if(isNull()) Error("LocalOp is null");
        return *L_;
        }

    const Tensor&
    R() const 
        { 
        if(isNull()) Error("LocalOp is null");
        return *R_;
        }

    const Tensor&
    bondTensor() const 
        { 
        makeBond();
        return bond_;
        }

    bool
    combineMPO() const { return combine_mpo_; }
    void
    combineMPO(bool val) { combine_mpo_ = val; }

    bool
    isNull() const { return Op1_ == 0; }
    bool
    isNotNull() const { return Op1_ != 0; }

    static LocalOp& Null()
        {
        static LocalOp Null_;
        return Null_;
        }

    void
    operator=(const LocalOp& other)
        {
        Op1_ = other.Op1_;
        Op2_ = other.Op2_;
        L_ = other.L_;
        R_ = other.R_;
        combine_mpo_ = other.combine_mpo_;
        bond_ = other.bond_;
        }

    private:

    /////////////////
    //
    // Data Members
    //

    const Tensor *Op1_, *Op2_; 
    const Tensor *L_, *R_; 
    bool combine_mpo_;
    mutable int size_;
    mutable Tensor bond_;

    //
    /////////////////

    void
    makeBond() const;

    };

template <class Tensor>
inline LocalOp<Tensor>::
LocalOp()
    :
    Op1_(0),
    Op2_(0),
    L_(0),
    R_(0),
    combine_mpo_(true),
    size_(-1)
    { 
    }

template <class Tensor>
inline LocalOp<Tensor>::
LocalOp(const Tensor& Op1, const Tensor& Op2, 
        const Tensor& L, const Tensor& R)
    : 
    Op1_(0),
    Op2_(0),
    L_(0),
    R_(0),
    combine_mpo_(true),
    size_(-1)
    {
    update(Op1,Op2,L,R);
    }

template <class Tensor>
void inline LocalOp<Tensor>::
update(const Tensor& Op1, const Tensor& Op2, 
       const Tensor& L, const Tensor& R)
    {
    Op1_ = &Op1;
    Op2_ = &Op2;
    L_ = &L;
    R_ = &R;
    size_ = -1;
    bond_ = Tensor();
    }

template <class Tensor>
inline void LocalOp<Tensor>::
product(const Tensor& phi, Tensor& phip) const
    {
    if(this->isNull()) Error("LocalOp is null");

    const Tensor& Op1 = *Op1_;
    const Tensor& Op2 = *Op2_;

    if(L().isNull())
        {
        phip = phi;

        if(R().isNotNull()) 
            phip *= R(); //m^3 k d

        if(combine_mpo_)
            {
            phip *= bondTensor();
            }
        else
            {
            phip *= Op2; //m^2 k^2
            phip *= Op1; //m^2 k^2
            }
        }
    else
        {
        phip = phi * L(); //m^3 k d

        if(combine_mpo_)
            {
            phip *= bondTensor();
            }
        else
            {
            phip *= Op1; //m^2 k^2
            phip *= Op2; //m^2 k^2
            }

        if(R().isNotNull()) 
            phip *= R();
        }

    phip.mapprime(1,0);
    }

template <class Tensor>
inline Real LocalOp<Tensor>::
expect(const Tensor& phi) const
    {
    Tensor phip;
    product(phi,phip);
    return Dot(phip,phi);
    }

template <class Tensor>
inline Tensor LocalOp<Tensor>::
deltaRho(const Tensor& AA, const CombinerT& comb, Direction dir) const
    {
    Tensor delta(AA);
    if(dir == Fromleft)
        {
        if(L().isNotNull()) delta *= L();
        delta *= (*Op1_);
        }
    else //dir == Fromright
        {
        if(R().isNotNull()) delta *= R();
        delta *= (*Op2_);
        }

    delta.noprime();
    delta = comb * delta;
    
    delta *= conj(primeind(delta,comb.right()));

    return delta;
    }

template <class Tensor>
inline Tensor LocalOp<Tensor>::
deltaPhi(const Tensor& phi) const
    {
    Tensor deltaL(phi),
           deltaR(phi);

    if(L().isNotNull()) 
        {
        deltaL *= L();
        }

    if(R().isNotNull()) 
        {
        deltaR *= R();
        }

    const Tensor& Op1 = *Op1_;
    const Tensor& Op2 = *Op2_;

    deltaL *= Op1;
    deltaR *= Op2;

    IndexT hl = index_in_common(Op1,Op2,Link);

    deltaL.trace(hl);
    deltaL.mapprime(1,0);

    deltaR.trace(hl);
    deltaR.mapprime(1,0);

    deltaL += deltaR;

    return deltaL;
    }

template <>
inline IQTensor LocalOp<IQTensor>::
deltaPhi(const IQTensor& phi) const
    {
    IQTensor deltaL(phi),
           deltaR(phi);

    if(L().isNotNull()) 
        {
        deltaL *= L();
        }

    if(R().isNotNull()) 
        {
        deltaR *= R();
        }

    const IQTensor& Op1 = *Op1_;
    const IQTensor& Op2 = *Op2_;

    deltaL *= Op1;
    deltaR *= Op2;

    IndexT hl = index_in_common(Op1,Op2,Link);

    deltaL.trace(hl);
    deltaL.mapprime(1,0);

    deltaR.trace(hl);
    deltaR.mapprime(1,0);

    deltaL += deltaR;

    std::vector<IQIndex> iqinds;
    iqinds.reserve(deltaL.r());
    for(int j = 1; j <= deltaL.r(); ++j)
        iqinds.push_back(deltaL.index(j));

    IQTensor delta(iqinds);

    QN targetQn = phi.div();

    Foreach(const ITensor& block, deltaL.itensors())
        {
        QN div;
        for(int j = 1; j <= block.r(); ++j)
            div += deltaL.qn(block.index(j));

        if(div == targetQn)
            delta += block;
        }

    return delta;
    }

template <class Tensor>
inline void LocalOp<Tensor>::
diag(Tensor& D) const
    {
    if(this->isNull()) Error("LocalOp is null");

    const Tensor& Op1 = *Op1_;
    const Tensor& Op2 = *Op2_;

    IndexT toTie;
    bool found = false;

    Tensor Diag = Op1;
    for(int j = 1; j <= Diag.r(); ++j)
        {
        const IndexT& s = Diag.index(j);
        if(s.primeLevel() == 0 && s.type() == Site) 
            {
            toTie = s;
            found = true;
            break;
            }
        }
    if(!found) Error("Couldn't find Index");
    Diag.tieIndices(toTie,primed(toTie),toTie);

    found = false;
    for(int j = 1; j <= Op2.r(); ++j)
        {
        const IndexT& s = Op2.index(j);
        if(s.primeLevel() == 0 && s.type() == Site) 
            {
            toTie = s;
            found = true;
            break;
            }
        }
    if(!found) Error("Couldn't find Index");
    Diag *= tieIndices(toTie,primed(toTie),toTie,Op2);

    if(L().isNotNull())
        {
        found = false;
        for(int j = 1; j <= L().r(); ++j)
            {
            const IndexT& ll = L().index(j);
            if(ll.primeLevel() == 0 && L().hasindex(primed(ll)))
                {
                toTie = ll;
                found = true;
                break;
                }
            }
        if(found)
            Diag *= tieIndices(toTie,primed(toTie),toTie,L());
        else
            Diag *= L();
        }

    if(R().isNotNull())
        {
        found = false;
        for(int j = 1; j <= R().r(); ++j)
            {
            const IndexT& rr = R().index(j);
            if(rr.primeLevel() == 0 && R().hasindex(primed(rr)))
                {
                toTie = rr;
                found = true;
                break;
                }
            }
        if(found)
            Diag *= tieIndices(toTie,primed(toTie),toTie,R());
        else
            Diag *= R();
        }

    D.assignFrom(Diag);
    }

template <class Tensor>
int LocalOp<Tensor>::
size() const
    {
    if(this->isNull()) Error("LocalOp is null");
    if(size_ == -1)
        {
        //Calculate linear size of this 
        //op as a square matrix
        size_ = 1;
        if(L().isNotNull()) 
            {
            size_ *= index_in_common(*Op1_,L(),Link).m();
            }
        if(R().isNotNull()) 
            {
            size_ *= index_in_common(*Op2_,R(),Link).m();
            }

            size_ *= Op1_->findtype(Site).m();
            size_ *= Op2_->findtype(Site).m();
        }
    return size_;
    }

template <class Tensor>
void LocalOp<Tensor>::
makeBond() const
    {
    if(bond_.isNull()) 
        {
        bond_ = (*Op1_) * (*Op2_);
        }
    }


#endif
