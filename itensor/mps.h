//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_MPS_H
#define __ITENSOR_MPS_H
#include "svdworker.h"
#include "model.h"
#include "option.h"

#define Cout std::cout
#define Endl std::endl
#define Format boost::format

template <class Tensor>
class MPOt;

template <class Tensor>
class LocalMPO;

static const LogNumber DefaultRefScale(7.58273202392352185);

void 
convertToIQ(const Model& model, const std::vector<ITensor>& A, 
            std::vector<IQTensor>& qA, QN totalq = QN(), Real cut = 1E-12);

class InitState
    {
    typedef IQIndexVal (*SetFuncPtr)(int);
    public:

    InitState(int nsite) 
        : N(nsite), 
          state(N+1) 
        { }

    InitState(int nsite,SetFuncPtr setter) 
        : N(nsite), 
          state(N+1) 
        { set_all(setter); }

    int NN() const { return N; }

    void 
    set_all(SetFuncPtr setter)
        { 
        for(int j = 1; j <= N; ++j) GET(state,j-1) = (*setter)(j); 
        }

    IQIndexVal& 
    operator()(int i) { return state.at(i-1); }
    const IQIndexVal& 
    operator()(int i) const { return state.at(i-1); }

    operator std::vector<IQIndexVal>() const { return state; }

    private:

    int N;
    std::vector<IQIndexVal> state;
    }; 

//
// class MPSt
// (the lowercase t stands for "template")
// 
// Unless there is a need to use MPSt
// specifically as a templated class,
// it is recommended to use one of the 
// typedefs of MPSt:
//
//      MPS for ITensors
//    IQMPS for IQTensors
//

template <class Tensor>
class MPSt
    {
    public:

    //
    //MPSt Constructors
    //

    MPSt();

    MPSt(const Model& mod_,int maxmm = MAX_M, Real cut = MIN_CUT);

    MPSt(const Model& mod_,const InitState& initState,
         int maxmm = MAX_M, Real cut = MIN_CUT);

    MPSt(const Model& model, std::istream& s);

    virtual 
    ~MPSt() { }

    //
    //MPSt Typedefs
    //

    typedef Tensor 
    TensorT;

    typedef typename Tensor::IndexT 
    IndexT;

    typedef typename Tensor::IndexValT 
    IndexValT;

    typedef typename Tensor::SparseT
    SparseT;

    typedef MPOt<Tensor>
    MPOType;

    //
    //MPSt Accessor Methods
    //

    int 
    NN() const { return N;}

    int 
    rightLim() const { return r_orth_lim_; }
    void 
    rightLim(int val) { r_orth_lim_ = val; }

    int 
    leftLim() const { return l_orth_lim_; }
    void 
    leftLim(int val) { l_orth_lim_ = val; }

    IQIndex 
    si(int i) const { return model_->si(i); }

    IQIndex 
    siP(int i) const { return model_->siP(i); }

    typedef typename std::vector<Tensor>::const_iterator 
    AA_it;

    const std::pair<AA_it,AA_it> 
    AA() const { return std::make_pair(A.begin()+1,A.end()); }

    const Tensor& 
    AA(int i) const 
        { 
        setSite(i);
        return A.at(i); 
        }

    Tensor& 
    AAnc(int i); //nc means 'non const'

    const Model& 
    model() const { return *model_; }

    const SVDWorker& 
    svd() const { return svd_; }
    SVDWorker& 
    svd() { return svd_; }


    bool 
    isNull() const { return (model_==0); }
    bool 
    isNotNull() const { return (model_!=0); }

    bool 
    doRelCutoff() const { return svd_.doRelCutoff(); }
    void 
    doRelCutoff(bool val) { svd_.doRelCutoff(val); }

    bool 
    absoluteCutoff() const { return svd_.absoluteCutoff(); }
    void 
    absoluteCutoff(bool val) { svd_.absoluteCutoff(val); }

    LogNumber 
    refNorm() const { return svd_.refNorm(); }
    void 
    refNorm(LogNumber val) { svd_.refNorm(val); }

    Real 
    noise() const { return svd_.noise(); }
    void 
    noise(Real val) { svd_.noise(val); }

    Real 
    cutoff() const { return svd_.cutoff(); }
    void 
    cutoff(Real val) { svd_.cutoff(val); }

    int 
    minm() const { return svd_.minm(); }
    void 
    minm(int val) { svd_.minm(val); }

    int 
    maxm() const { return svd_.maxm(); }
    void 
    maxm(int val) { svd_.maxm(val); }

    Real 
    truncerr(int b) const { return svd_.truncerr(b); }

    const Vector& 
    eigsKept(int b) const { return svd_.eigsKept(b); }

    bool 
    showeigs() const { return svd_.showeigs(); }
    void 
    showeigs(bool val) { svd_.showeigs(val); }

    Tensor 
    bondTensor(int b) const;

    bool
    doWrite() const { return do_write_; }
    void
    doWrite(bool val) 
        { 
        if(!do_write_ && (val == true))
            initWrite(); 
        do_write_ = val;
        }

    const std::string&
    writeDir() const { return writedir_; }

    bool 
    isOrtho() const { return is_ortho_; }
    //Only use the following method if
    //you are sure of what you are doing!
    void 
    isOrtho(bool val) { is_ortho_ = val; }


    void 
    read(std::istream& s);
    void 
    write(std::ostream& s) const;

    //Read from a directory containing individual tensors,
    //as created when doWrite(true) is called.
    void 
    read(const std::string& dirname);

    //
    //MPSt Operators
    //

    MPSt& 
    operator*=(Real a) { AAnc(l_orth_lim_+1) *= a; return *this; }

    MPSt 
    operator*(Real r) const { MPSt res(*this); res *= r; return res; }

    friend inline MPSt 
    operator*(Real r, MPSt res) { res *= r; return res; }

    MPSt& 
    operator+=(const MPSt& oth);
    MPSt& 
    addNoOrth(const MPSt& oth);

    inline MPSt 
    operator+(MPSt res) const { res += *this; return res; }

    inline MPSt 
    operator-(MPSt res) const { res *= -1; res += *this; return res; }

    //
    //MPSt Index Methods
    //

    void 
    mapprime(int oldp, int newp, PrimeType pt = primeBoth);

    void 
    primelinks(int oldp, int newp);

    void 
    noprimelink();

    IndexT 
    LinkInd(int b) const 
        { return index_in_common(AA(b),AA(b+1),Link); }
    IndexT 
    RightLinkInd(int i) const 
        { return index_in_common(AA(i),AA(i+1),Link); }
    IndexT 
    LeftLinkInd(int i)  const 
        { return index_in_common(AA(i),AA(i-1),Link); }

    //
    //MPSt orthogonalization methods
    //

    void 
    svdBond(int b, const Tensor& AA, Direction dir, 
            const Option& opt = Option());

    template <class LocalOpT>
    void 
    svdBond(int b, const Tensor& AA, Direction dir, 
                const LocalOpT& PH, const Option& opt = Option());

    void
    doSVD(int b, const Tensor& AA, Direction dir, const Option& opt = Option())
        { 
        svdBond(b,AA,dir,opt); 
        }

    //Move the orthogonality center to site i 
    //(l_orth_lim_ = i-1, r_orth_lim_ = i+1)
    void 
    position(int i, const Option& opt = Option());

    int 
    orthoCenter() const 
        { 
        if(!isOrtho()) Error("orthogonality center not well defined.");
        return (l_orth_lim_ + 1);
        }

    void 
    orthogonalize(const Option& opt = Option());

    //Checks if A[i] is left (left == true) 
    //or right (left == false) orthogonalized
    bool 
    checkOrtho(int i, bool left) const;

    bool 
    checkRightOrtho(int i) const { return checkOrtho(i,false); }

    bool 
    checkLeftOrtho(int i) const { return checkOrtho(i,true); }
    
    bool 
    checkOrtho() const;


    //
    // projectOp takes the projected edge tensor W 
    // of an operator and the site tensor X for the operator
    // and creates the next projected edge tensor nE
    //
    // dir==Fromleft example:
    //
    //  /---A--     /---
    //  |   |       |
    //  E-- X -  =  nE -
    //  |   |       |
    //  \---A--     \---
    //
    void 
    projectOp(int j, Direction dir, 
              const Tensor& E, const Tensor& X, Tensor& nE) const;


    //
    // Applies a bond gate to the bond that is currently
    // the OC.                                    |      |
    // After calling position b, this bond is - A[b] - A[b+1] -
    //
    //      |      |
    //      ==gate==
    //      |      |
    //  - A[b] - A[b+1] -
    //
    void 
    applygate(const Tensor& gate);

    Real 
    norm() const { return sqrt(psiphi(*this,*this)); }

    int
    averageM() const;

    Real 
    normalize()
        {
        Real norm_ = norm();
        if(fabs(norm_) < 1E-20) Error("Zero norm");
        *this *= 1.0/norm_;
        return norm_;
        }

    bool 
    isComplex() const
        { return A[l_orth_lim_+1].isComplex(); }

    friend inline std::ostream& 
    operator<<(std::ostream& s, const MPSt& M)
        {
        s << "\n";
        for(int i = 1; i <= M.NN(); ++i) s << M.AA(i) << "\n";
        return s;
        }

    void print(std::string name = "",Printdat pdat = HideData) const 
        { 
        bool savep = Global::printdat();
        Global::printdat() = (pdat==ShowData); 
        std::cerr << "\n" << name << " =\n" << *this << "\n"; 
        Global::printdat() = savep;
        }

    void 
    printIndices(const std::string& name = "") const
        {
        Cout << name << "=" << Endl;
        for(int i = 1; i <= NN(); ++i) 
            AA(i).printIndices(boost::format("AA(%d)")%i);
        }

    void 
    printIndices(const boost::format& fname) const
        { printIndices(fname.str()); }

    void 
    toIQ(QN totalq, MPSt<IQTensor>& iqpsi, Real cut = 1E-12) const
        {
        iqpsi = MPSt<IQTensor>(*model_,maxm(),cutoff());
        iqpsi.svd_ = svd_;
        convertToIQ(*model_,A,iqpsi.A,totalq,cut);
        }

protected:

    //////////////////////////
    //
    //Data Members

    int N;

    mutable
    std::vector<Tensor> A;

    int l_orth_lim_,
        r_orth_lim_;

    bool is_ortho_;

    const Model* model_;

    SVDWorker svd_;

    mutable
    int atb_;

    std::string writedir_;

    bool do_write_;

    //
    //////////////////////////

    //
    //MPSt methods for writing to disk
    //

    //if doWrite(true) is called
    //setBond(b) loads bond b
    //from disk, keeping all other
    //tensors written to disk

    void
    setBond(int b) const;

    void
    setSite(int j) const
        {
        if(!do_write_)
            {
            atb_ = (j > atb_ ? j-1 : j);
            return;
            }

        if(j < atb_)
            setBond(j);
        else
        if(j > atb_+1)
            setBond(j-1);

        //otherwise the set bond already
        //contains this site
        }


    void
    initWrite();

    std::string
    AFName(int j) const;

    //
    //Constructor Helpers
    //

    void 
    new_tensors(std::vector<ITensor>& A_);

    void 
    random_tensors(std::vector<ITensor>& A_);

    void 
    random_tensors(std::vector<IQTensor>& A_) { }

    void 
    init_tensors(std::vector<ITensor>& A_, const InitState& initState);

    void 
    init_tensors(std::vector<IQTensor>& A_, const InitState& initState);

private:
    friend class MPSt<ITensor>;
    friend class MPSt<IQTensor>;
}; //class MPSt<Tensor>
typedef MPSt<ITensor> MPS;
typedef MPSt<IQTensor> IQMPS;

//
// MPSt
// Template Methods
//

template <class Tensor>
template <class LocalOpT>
void MPSt<Tensor>::
svdBond(int b, const Tensor& AA, Direction dir, 
            const LocalOpT& PH, const Option& opt)
    {
    setBond(b);
    if(opt == PreserveShape())
        {
        //The idea of the preserve_shape flag is to 
        //leave any external indices of the MPS on the
        //tensors they originally belong to
        Error("preserve_shape not currently implemented");
        }

    if(dir == Fromleft && b-1 > l_orth_lim_)
        {
        Cout << Format("b=%d, l_orth_lim_=%d")
                %b%l_orth_lim_ << Endl;
        Error("b-1 > l_orth_lim_");
        }
    if(dir == Fromright && b+2 < r_orth_lim_)
        {
        Cout << Format("b=%d, r_orth_lim_=%d")
                %b%r_orth_lim_ << Endl;
        Error("b+2 < r_orth_lim_");
        }

#define USE_SVD_ONLY

#ifdef USE_SVD_ONLY
    {
    SparseT D;
    svd_.svd(b,AA,A[b],D,A[b+1]);

    //Normalize the orthogonality center
    //if(opt.boolEquals(DoNormalize(true)))
    //    {
    //    Real norm = D.norm();
    //    D *= 1./norm;
    //    }

    //Push the singular values into the appropriate site tensor
    if(dir == Fromleft)
        A[b+1] *= D;
    else
        A[b] *= D;
    }
#else
    if(cutoff() < 1E-12)
        {
        //Need high accuracy, use svd which calls the
        //accurate SVD method in the MatrixRef library
        SparseT D;
        svd_.svd(b,AA,A[b],D,A[b+1]);

        //Normalize the orthogonality center
        //if(opt.boolEquals(DoNormalize(true)))
        //    {
        //    Real norm = D.norm();
        //    D *= 1./norm;
        //    }

        //Push the singular values into the appropriate site tensor
        if(dir == Fromleft)
            A[b+1] *= D;
        else
            A[b] *= D;
        }
    else
        {
        //If we don't need extreme accuracy,
        //use presumably faster density matrix approach
        svd_.denmatDecomp(b,AA,A[b],A[b+1],dir,PH);

        //Normalize the ortho center
        //if(opt.boolEquals(DoNormalize(true)))
        //    {
        //    Tensor& oc = (dir == Fromleft ? A[b+1] : A[b]);
        //    Real norm = oc.norm();
        //    oc *= 1./norm;
        //    }
        }
#endif

    if(dir == Fromleft)
        {
        l_orth_lim_ = b;
        if(r_orth_lim_ < b+2) r_orth_lim_ = b+2;
        }
    else //dir == Fromright
        {
        if(l_orth_lim_ > b-1) l_orth_lim_ = b-1;
        r_orth_lim_ = b+1;
        }
    }

//
// Other Methods Related to MPSt
//

int 
findCenter(const IQMPS& psi);

inline bool 
checkQNs(const MPS& psi) { return true; }

bool 
checkQNs(const IQMPS& psi);

inline QN 
totalQN(const IQMPS& psi)
    {
    int center = findCenter(psi);
    if(center == -1)
        Error("Could not find ortho. center");
    return psi.AA(center).div();
    }

//
// <psi | phi>
//
template <class MPSType>
void 
psiphi(const MPSType& psi, const MPSType& phi, Real& re, Real& im)
    {
    typedef typename MPSType::TensorT
    Tensor;

    const int N = psi.NN();
    if(N != phi.NN()) Error("psiphi: mismatched N");

    Tensor L = phi.AA(1) * conj(primeind(psi.AA(1),psi.LinkInd(1))); 

    for(int i = 2; i < psi.NN(); ++i) 
        { 
        L = L * phi.AA(i) * conj(primelink(psi.AA(i))); 
        }
    L = L * phi.AA(N);

    BraKet(primeind(psi.AA(N),psi.LinkInd(N-1)),L,re,im);
    }

template <class MPSType>
Real 
psiphi(const MPSType& psi, const MPSType& phi) //Re[<psi|phi>]
    {
    Real re, im;
    psiphi(psi,phi,re,im);
    if(im != 0) 
	if(fabs(im) > 1.0e-12 * fabs(re))
	    std::cerr << "Real psiphi: WARNING, dropping non-zero imaginary part of expectation value.\n";
    return re;
    }

//Computes an MPS which has the same overlap with psi_basis as psi_to_fit,
//but which differs from psi_basis only on the first site, and has same index
//structure as psi_basis. Result is stored to psi_to_fit on return.
void 
fitWF(const IQMPS& psi_basis, IQMPS& psi_to_fit);

//Template method for efficiently summing a set of MPS's or MPO's 
//(or any class supporting operator+=)
template <typename MPSType>
void 
sum(const std::vector<MPSType>& terms, MPSType& res, 
    Real cut = MIN_CUT, int maxm = MAX_M)
    {
    const int Nt = terms.size();
    if(Nt == 2)
        { 
        res = terms[0];
        res.cutoff(cut); 
        res.maxm(maxm);
        //std::cerr << boost::format("Before +=, cutoff = %.1E, maxm = %d\n")%(res.cutoff())%(res.maxm());
        res += terms[1];
        }
    else 
    if(Nt == 1) 
        {
        res = terms[0];
        res.cutoff(cut); 
        res.maxm(maxm);
        }
    else 
    if(Nt > 2)
        {
        //Add all MPS's in pairs
        const int nsize = (Nt%2==0 ? Nt/2 : (Nt-1)/2+1);
        std::vector<MPSType> tpair(2), 
                             newterms(nsize); 
        for(int n = 0, np = 0; n < Nt-1; n += 2, ++np)
            {
            tpair[0] = terms[n]; 
            tpair[1] = terms[n+1];
            sum(tpair,newterms.at(np),cut,maxm);
            }
        if(Nt%2 == 1) newterms.at(nsize-1) = terms.back();

        //Recursively call sum again
        sum(newterms,res,cut,maxm);
        }
    }

#undef Cout
#undef Endl
#undef Format

#endif
