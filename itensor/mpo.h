//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_MPO_H
#define __ITENSOR_MPO_H
#include "mps.h"

//
// class MPOt
//
// (defines MPO and IQMPO via typedefs)
//
template<class Tensor>
class MPOt : private MPSt<Tensor>
    {
    public:

    typedef MPSt<Tensor> 
    Parent;

    typedef Tensor 
    TensorT;

    typedef typename Tensor::IndexT 
    IndexT;

    typedef typename Tensor::SparseT
    SparseT;

    typedef typename Tensor::IndexValT 
    IndexValT;

    typedef typename Tensor::CombinerT 
    CombinerT;

    //MPOt: Constructors -----------------------------------------

    MPOt() 
        : 
        Parent() 
        { doRelCutoff(true); }

    MPOt(const Model& model, int maxm_ = MAX_M, Real cutoff_ = MIN_CUT, 
         bool _doRelCutoff = true, LogNumber _refNorm = DefaultRefScale) 
        : 
        Parent(model,maxm_,cutoff_)
        { 
        doRelCutoff(_doRelCutoff);
        refNorm(_refNorm);

        // Norm of psi^2 = 1 = norm = sum of denmat evals. 
        // This translates to Tr{Adag A} = norm.  
        // Ref. norm is Tr{1} = d^N, d = 2 S=1/2, d = 4 for Hubbard, etc
        if(_refNorm == DefaultRefScale) 
            refNorm(exp(model.NN()));
        }

    MPOt(Model& model, std::istream& s) { read(model,s); }

    //Accessor Methods ------------------------------

    using Parent::NN;

    using Parent::model;
    using Parent::isNull;
    using Parent::isNotNull;

    using Parent::si;
    using Parent::siP;

    using Parent::rightLim;
    using Parent::leftLim;

    using Parent::AA;
    using Parent::AAnc;
    using Parent::bondTensor;

    using Parent::doWrite;
    using Parent::doRelCutoff;
    using Parent::refNorm;
    using Parent::cutoff;
    using Parent::minm;
    using Parent::maxm;
    using Parent::truncerr;
    using Parent::eigsKept;
    using Parent::showeigs;
    using Parent::svd;


    using Parent::read;
    using Parent::write;

    //MPOt: operators ------------------------------------------------------

    MPOt& 
    operator*=(Real a) { Parent::operator*=(a); return *this; }

    MPOt
    operator*(Real r) const { MPOt res(*this); res *= r; return res; }

    friend MPOt inline
    operator*(Real r, MPOt res) { res *= r; return res; }

    MPOt&
    addNoOrth(const MPOt& oth) { Parent::addNoOrth(oth); return *this; }

    MPOt& 
    operator+=(const MPOt& oth);


    MPOt 
    operator+(MPOt res) const { res += *this; return res; }

    MPOt 
    operator-(MPOt res) const { res *= -1; res += *this; return res; }

    operator MPOt<IQTensor>()
        { 
        MPOt<IQTensor> res(*model_,maxm(),cutoff(),doRelCutoff(),refNorm()); 
        res.svd_ = svd_;
        convertToIQ(*model_,A,res.A);
        return res; 
        }

    MPOt<ITensor>
    toMPO() const;

    //MPOt: index methods --------------------------------------------------

    using Parent::mapprime;
    using Parent::primelinks;
    using Parent::noprimelink;

    using Parent::LinkInd;
    using Parent::RightLinkInd;
    using Parent::LeftLinkInd;

    void 
    primeall()	// sites i,i' -> i',i'';  link:  l -> l'
        {
        for(int i = 1; i <= this->NN(); i++)
            {
            AAnc(i).mapprime(0,1,primeLink);
            AAnc(i).mapprime(1,2,primeSite);
            AAnc(i).mapprime(0,1,primeSite);
            }
        }

    void 
    svdBond(int b, const Tensor& AA, Direction dir, const Option& opt = Option());

    void
    doSVD(int b, const Tensor& AA, Direction dir, const Option& opt = Option())
        { 
        svdBond(b,AA,dir,opt); 
        }

    //Move the orthogonality center to site i 
    //(l_orth_lim_ = i-1, r_orth_lim_ = i+1)
    void 
    position(int i, const Option& opt = Option());

    void 
    orthogonalize(const Option& opt = Option());

    using Parent::isOrtho;
    using Parent::orthoCenter;
    using Parent::orthogonalize;

    using Parent::isComplex;

    using Parent::averageM;

    using Parent::applygate;

    friend inline std::ostream& 
    operator<<(std::ostream& s, const MPOt& M)
        {
        s << "\n";
        for(int i = 1; i <= M.NN(); ++i) s << M.AA(i) << "\n";
        return s;
        }

    using Parent::print;
    using Parent::printIndices;

    void 
    toIQ(QN totalq, MPOt<IQTensor>& res, Real cut = 1E-12) const
        {
        res = MPOt<IQTensor>(*model_,maxm(),cutoff());
        res.svd_ = svd_;
        convertToIQ(*model_,A,res.A,totalq,cut);
        }

private:
    using Parent::N;
    using Parent::A;
    using Parent::l_orth_lim_;
    using Parent::r_orth_lim_;
    using Parent::model_;
    using Parent::svd_;
    using Parent::is_ortho_;

    friend class MPOt<ITensor>;
    friend class MPOt<IQTensor>;
    }; //class MPOt<Tensor>
typedef MPOt<ITensor> MPO;
typedef MPOt<IQTensor> IQMPO;

template <> inline
MPO MPOt<IQTensor>::
toMPO() const
    {
    MPO res(*model_,maxm(),cutoff(),doRelCutoff(),refNorm());
    res.svd_ = svd_;
    for(int j = 1; j <= NN(); ++j)
        {
        res.A.at(j) = AA(j).toITensor();
        }
    return res;
    }

//toMPO method fails unless template class 
//Tensor is set to IQTensor (object is an IQMPO)
template<class Tensor>
MPO MPOt<Tensor>::
toMPO() const
    {
    Error("toMPO only implemented for class IQMPO");
    return MPO();
    }


int
findCenter(const IQMPO& psi);

void inline 
checkQNs(const MPO& psi) { }

void
checkQNs(const IQMPO& psi);

namespace Internal {

template<class Tensor>
class MPOSet
    {
    public:

    typedef std::vector<Tensor> 
    TensorT;

    MPOSet() 
        : 
        N(-1), 
        size_(0) 
        { }

    MPOSet(const MPOt<Tensor>& Op1) 
        : 
        N(-1), 
        size_(0) 
        { include(Op1); }

    MPOSet(const MPOt<Tensor>& Op1, 
           const MPOt<Tensor>& Op2) 
        : 
        N(-1), 
        size_(0) 
        { include(Op1); include(Op2); }

    MPOSet(const MPOt<Tensor>& Op1, 
           const MPOt<Tensor>& Op2,
           const MPOt<Tensor>& Op3) 
        : 
        N(-1), 
        size_(0) 
        { include(Op1); include(Op2); include(Op3); }

    MPOSet(const MPOt<Tensor>& Op1, 
           const MPOt<Tensor>& Op2,
           const MPOt<Tensor>& Op3, 
           const MPOt<Tensor>& Op4) 
        : 
        N(-1), 
        size_(0) 
        { include(Op1); include(Op2); include(Op3); include(Op4); }

    void 
    include(const MPOt<Tensor>& Op)
        {
        if(N < 0) 
            { 
            N = Op.NN(); 
            A.resize(N+1); 
            }
        for(int n = 1; n <= N; ++n) 
            A[n].push_back(&(Op.AA(n))); 
        ++size_;
        }

    int 
    NN() const { return N; }

    int 
    size() const { return size_; }

    const std::vector<const Tensor*>& 
    AA(int j) const 
        { 
        return A.at(j); 
        }

    const std::vector<Tensor> 
    bondTensor(int b) const
        { 
        std::vector<Tensor> res = A[b] * A[b+1]; 
        return res; 
        }

    private:

    ////////////
    //
    // Data Members
    //

    int N, 
        size_;

    std::vector<std::vector<const Tensor*> > 
    A;

    //
    ////////////

    }; //class Internal::MPOSet

} //namespace Internal
typedef Internal::MPOSet<ITensor> MPOSet;
typedef Internal::MPOSet<IQTensor> IQMPOSet;

template <class MPSType, class MPOType>
void 
psiHphi(const MPSType& psi, const MPOType& H, const MPSType& phi, Real& re, Real& im) //<psi|H|phi>
    {
    typedef typename MPSType::TensorT Tensor;
    const int N = H.NN();
    if(phi.NN() != N || psi.NN() != N) Error("psiHphi: mismatched N");

    Tensor L = phi.AA(1); 
    L *= H.AA(1); 
    L *= conj(primed(psi.AA(1)));
    for(int i = 2; i < N; ++i) 
        { 
        L *= phi.AA(i); 
        L *= H.AA(i); 
        L *= conj(primed(psi.AA(i))); 
        }
    L *= phi.AA(N); L *= H.AA(N);

    BraKet(primed(psi.AA(N)),L,re,im);
    }
template <class MPSType, class MPOType>
Real 
psiHphi(const MPSType& psi, const MPOType& H, const MPSType& phi) //Re[<psi|H|phi>]
    {
    Real re, im;
    psiHphi(psi,H,phi,re,im);
    if(fabs(im) > 1.0e-12 * fabs(re))
	std::cerr << boost::format("\nReal psiHphi: WARNING, dropping non-zero (im = %.5f) imaginary part of expectation value.\n")%im;
    return re;
    }

void inline
psiHphi(const MPS& psi, const MPO& H, const ITensor& LB, const ITensor& RB, const MPS& phi, Real& re, Real& im) //<psi|H|phi>
    {
    int N = psi.NN();
    if(N != phi.NN() || H.NN() < N) Error("mismatched N in psiHphi");
    MPS psiconj(psi);
    for(int i = 1; i <= N; ++i) 
        psiconj.AAnc(i) = conj(primed(psi.AA(i)));
    ITensor L = (LB.isNull() ? phi.AA(1) : LB * phi.AA(1));
    L *= H.AA(1); L *= psiconj.AA(1);
    for(int i = 2; i <= N; ++i)
        { L *= phi.AA(i); L *= H.AA(i); L *= psiconj.AA(i); }
    if(!RB.isNull()) L *= RB;
    if(L.isComplex())
        {
        if(L.vecSize() != 2) Error("Non-scalar result in psiHphi.");
        re = L(Index::IndReIm()(1));
        im = L(Index::IndReIm()(2));
        }
    else 
        {
        if(L.vecSize() != 1) Error("Non-scalar result in psiHphi.");
        re = L.val0();
        im = 0;
        }
    }
Real inline
psiHphi(const MPS& psi, const MPO& H, const ITensor& LB, const ITensor& RB, const MPS& phi) //Re[<psi|H|phi>]
    {
    Real re,im; psiHphi(psi,H,LB,RB,phi,re,im);
    if(fabs(im) > 1.0e-12 * fabs(re))
	std::cerr << "Real psiHphi: WARNING, dropping non-zero imaginary part of expectation value.\n";
    return re;
    }

void inline 
psiHKphi(const IQMPS& psi, const IQMPO& H, const IQMPO& K,const IQMPS& phi, Real& re, Real& im) //<psi|H K|phi>
    {
    if(psi.NN() != phi.NN() || psi.NN() != H.NN() || psi.NN() != K.NN()) Error("Mismatched N in psiHKphi");
    int N = psi.NN();
    IQMPS psiconj(psi);
    for(int i = 1; i <= N; i++)
        {
        psiconj.AAnc(i) = conj(psi.AA(i));
        psiconj.AAnc(i).mapprime(0,2);
        }
    IQMPO Kp(K);
    Kp.mapprime(1,2);
    Kp.mapprime(0,1);

    //scales as m^2 k^2 d
    IQTensor L = (((phi.AA(1) * H.AA(1)) * Kp.AA(1)) * psiconj.AA(1));
    for(int i = 2; i < N; i++)
        {
        //scales as m^3 k^2 d + m^2 k^3 d^2
        L = ((((L * phi.AA(i)) * H.AA(i)) * Kp.AA(i)) * psiconj.AA(i));
        }
    //scales as m^2 k^2 d
    L = ((((L * phi.AA(N)) * H.AA(N)) * Kp.AA(N)) * psiconj.AA(N)) * IQTensor::Sing();
    //cout << "in psiHKpsi, L is "; PrintDat(L);
    L.GetSingComplex(re,im);
    }

Real inline 
psiHKphi(const IQMPS& psi, const IQMPO& H, const IQMPO& K,const IQMPS& phi) //<psi|H K|phi>
    {
    Real re,im;
    psiHKphi(psi,H,K,phi,re,im);
    if(fabs(im) > 1.0e-12 * fabs(re))
	Error("Non-zero imaginary part in psiHKphi");
    return re;
    }

template <class MPOType>
void 
nmultMPO(const MPOType& Aorig, const MPOType& Borig, MPOType& res,Real cut, int maxm);

//
// Applies an MPO to an MPS using the zip-up method described
// more fully in Stoudenmire and White, New. J. Phys. 12, 055026 (2010).
//
// This method applies the MPO to an MPS one site at a time,
// with the new MPS being calculated at each step via an
// SVD of the MPO-MPS product.
//
// Uses cutoff and max of MPS psi unless specified.
//
template<class Tensor>
void 
zipUpApplyMPO(const MPSt<Tensor>& psi, const MPOt<Tensor>& K, MPSt<Tensor>& res, Real cutoff = -1, int maxm = -1);

template<class Tensor>
void 
napplyMPO(const MPSt<Tensor>& psi, const MPOt<Tensor>& K, MPSt<Tensor>& res, 
          Real cutoff = -1, int maxm = -1, bool allow_arb_position = false)
    {
    static int count = 0;
    if(count++ < 10)
        std::cout << "\n\n\nWarning: function name napplyMPO deprecated, use zipUpApplyMPO instead\n\n\n" << std::endl;
    zipUpApplyMPO(psi,K,res,cutoff,maxm);
    }

//Applies an MPO K to an MPS x with no approximation (|res>=K|x>)
//The bond dimension of res will be the product of bond dimensions
//of x and K.
template<class Tensor>
void 
exactApplyMPO(const MPSt<Tensor>& x, const MPOt<Tensor>& K, MPSt<Tensor>& res);

//Computes the exponential of the MPO H: K=exp(-tau*(H-Etot))
template<class Tensor>
void 
expH(const MPOt<Tensor>& H, MPOt<Tensor>& K, Real tau, Real Etot,
     Real Kcutoff, int ndoub);

#endif
