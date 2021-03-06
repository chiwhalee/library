//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_ITSPARSE_H
#define __ITENSOR_ITSPARSE_H
#include "itensor.h"

//
// ITSparse
//
class ITSparse
    {
    public:

    //
    // Constructors
    //

    ITSparse();

    explicit
    ITSparse(const Index& i1);

    ITSparse(const Index& i1, Real d);

    ITSparse(const Index& i1, const Vector& diag);

    ITSparse(const Index& i1, const Index& i2, Real d);

    ITSparse(const Index& i1, const Index& i2, const Vector& diag);

    ITSparse(const Index& i1, const Index& i2, 
                  const Index& i3, Real d);

    ITSparse(const Index& i1, const Index& i2, 
                  const Index& i3, const Vector& diag);


    ITSparse(const Index& i1, const Index& i2, 
                  const Index& i3, const Index& i4, Real d);

    explicit
    ITSparse(std::istream& s) { read(s); }

    //
    // Accessor Methods
    //

    const Index& 
    index(int j) const { return is_.index(j); }

    int 
    r() const { return is_.r_; }

    int 
    rn() const { return is_.rn_; }

    int 
    m(int j) const { return is_.m(j); }

    //uniqueReal depends on indices only, unordered:
    Real 
    uniqueReal() const { return is_.ur_; } 

    bool 
    isComplex() const { return hasindexn(Index::IndReIm()); }

    bool 
    isNotComplex() const { return !hasindexn(Index::IndReIm()); }

    const LogNumber&
    scale() const { return scale_; }

    bool
    isDiag() const { return true; }

    bool
    diagAllSame() const { return diag_.Length() == 0; }

    int
    diagSize() const;

    Vector
    diag() const;

    void
    diag(VectorRef v);

    //
    // Operators
    //

    // Contracting product with ITensor

    ITensor
    operator*(const ITensor& T) const
        { ITensor res; product(*this,T,res); return res; }

    friend inline ITensor&
    operator*=(ITensor& T, const ITSparse& S)
        { ITensor res; product(S,T,res); T.swap(res); return T; }

    ITensor friend inline
    operator*(const ITensor& T, const ITSparse& S)
        { ITensor res; product(S,T,res); return res; }

    // Addition and subtraction

    ITSparse&
    operator+=(const ITSparse& other);

    ITSparse
    operator+(const ITSparse& other)
        { ITSparse res(*this); res += other; return res; }

    ITSparse&
    operator-=(const ITSparse& other);

    ITSparse
    operator-(const ITSparse& other)
        { ITSparse res(*this); res -= other; return res; }

    // Multiplication and division by scalars

    ITSparse& 
    operator*=(Real fac) { scale_ *= fac; return *this; }

    ITSparse 
    operator*(Real fac) const 
        { ITSparse res(*this); res *= fac; return res; }

    friend inline ITSparse 
    operator*(Real fac, ITSparse s) 
        { return (s *= fac); }

    ITSparse& 
    operator/=(Real fac) { scale_ /= fac; return *this; }

    ITSparse 
    operator/(Real fac) const 
        { ITSparse res(*this); res /= fac; return res; }

    friend inline ITSparse 
    operator/(Real fac, ITSparse s) 
        { return (s /= fac); }

    ITSparse& 
    operator*=(const LogNumber& fac) { scale_ *= fac; return *this; }

    ITSparse 
    operator*(const LogNumber& fac) const 
        { ITSparse res(*this); res *= fac; return res; }

    friend inline ITSparse 
    operator*(const LogNumber& fac, ITSparse s) 
        { return (s *= fac); }

    //
    // Index Methods
    //

    Index 
    findtype(IndexType t) const { return is_.findtype(t); }

    bool 
    findtype(IndexType t, Index& I) const { return is_.findtype(t,I); }

    int 
    findindex(const Index& I) const { return is_.findindex(I); }

    int 
    findindexn(const Index& I) const { return is_.findindexn(I); }

    int 
    findindex1(const Index& I) const { return is_.findindex1(I); }

    bool 
    has_common_index(const ITensor& other) const
        { return is_.has_common_index(other.is_); }
    
    bool 
    hasindex(const Index& I) const { return is_.hasindex(I); }

    bool 
    hasindexn(const Index& I) const { return is_.hasindexn(I); }

    bool 
    hasindex1(const Index& I) const { return is_.hasindex1(I); }

    bool
    hasAllIndex(const boost::array<Index,NMAX+1>& I, int nind) const
        { return is_.hasAllIndex(I,nind); }

    bool 
    notin(const Index& I) const { return !hasindex(I); }

    void 
    addindex1(const std::vector<Index>& indices) { is_.addindex1(indices); }

    void 
    addindex1(const Index& I) { is_.addindex1(I); }

    //Removes the jth index as found by findindex
    void 
    removeindex1(int j) { is_.removeindex1(j); }

    void 
    removeindex1(const Index& I) { is_.removeindex1(is_.findindex1(I)); }

    void 
    mapindex(const Index& i1, const Index& i2) { is_.mapindex(i1,i2); }

    //
    // Primelevel Methods 
    //

    void 
    noprime(PrimeType p = primeBoth) { is_.noprime(p); }

    void 
    doprime(PrimeType pt, int inc = 1) { is_.doprime(pt,inc); }

    void 
    primeall() { doprime(primeBoth,1); }

    void 
    primesite() { doprime(primeSite,1); }

    void 
    primelink() { doprime(primeLink,1); }

    void 
    mapprime(int plevold, int plevnew, PrimeType pt = primeBoth)
        { is_.mapprime(plevold,plevnew,pt); }

    void 
    mapprimeind(const Index& I, int plevold, int plevnew, 
                PrimeType pt = primeBoth)
        { is_.mapprimeind(I,plevold,plevnew,pt); }

    void 
    primeind(const Index& I, int inc = 1)
        { mapindex(I,primed(I,inc)); }

    void 
    primeind(const Index& I, const Index& J) { is_.primeind(I,J); }

    void 
    noprimeind(const Index& I) { mapindex(I,I.deprimed()); }

    friend inline ITSparse
    primed(ITSparse S, int inc = 1)
        { S.doprime(primeBoth,inc); return S; }

    friend inline ITSparse
    primesite(ITSparse S, int inc = 1)
        { S.doprime(primeSite,inc); return S; }

    friend inline ITSparse
    primelink(ITSparse S, int inc = 1)
        { S.doprime(primeLink,inc); return S; }

    friend inline ITSparse
    primeind(ITSparse S, const Index& I, int inc = 1)
        { S.primeind(I,inc); return S; }

    friend inline ITSparse
    deprimed(ITSparse S)
        { S.noprime(); return S; }

    //
    // Other Methods
    //

    template <typename Callable> void
    mapElems(const Callable& f);

    void
    pseudoInvert(Real cutoff = 0);

    Real
    norm() const;

    void 
    scaleOutNorm() const;

    void 
    scaleTo(LogNumber newscale) const;

    void
    read(std::istream& s);

    void
    write(std::ostream& s) const;

    void
    conj() { }

    bool
    isNull() const { return (scale_ == LogNumber(0) && diag_.Length() == 0); }

    bool
    isNotNull() const { return (scale_ != LogNumber(0) || diag_.Length() != 0); }

    void 
    print(std::string name = "",Printdat pdat = HideData) const;

    void 
    printIndices(const std::string& name = "") const
        { print(name,HideData); }

    void 
    printIndices(const boost::format& fname) const
        { printIndices(fname.str()); }

    friend std::ostream&
    operator<<(std::ostream & s, const ITSparse & t);

    typedef Index 
    IndexT;

    typedef IndexVal 
    IndexValT;

    typedef Combiner 
    CombinerT;

    private:

    //////////////
    //
    // Data members
    //

    //diagonal elements
    mutable Vector diag_;

    IndexSet is_;

    mutable LogNumber scale_;

    //
    //////////////

    void
    _construct1(const Index& i1);

    void
    _construct2(const Index& i1, const Index& i2);

    friend void 
    product(const ITSparse& S, const ITensor& T, ITensor& res);

    }; // class ITSparse

void 
product(const ITSparse& S, const ITensor& T, ITensor& res);

template <typename Callable> void ITSparse::
mapElems(const Callable& f)
    {
    scaleTo(1);
    for(int j = 1; j <= diag_.Length(); ++j)
        diag_(j) = f(diag_(j));
    }

#endif
