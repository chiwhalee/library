//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#include "hambuilder.h"

using namespace std;
using boost::format;

template<class Tensor> 
void MPOt<Tensor>::
position(int i, const Option& opt)
    {
    if(isNull()) Error("position: MPS is null");

    while(l_orth_lim_ < i-1)
        {
        if(l_orth_lim_ < 0) l_orth_lim_ = 0;
        Tensor WF = AA(l_orth_lim_+1) * AA(l_orth_lim_+2);
        svdBond(l_orth_lim_+1,WF,Fromleft,opt);
        }
    while(r_orth_lim_ > i+1)
        {
        if(r_orth_lim_ > N+1) r_orth_lim_ = N+1;
        Tensor WF = AA(r_orth_lim_-2) * AA(r_orth_lim_-1);
        svdBond(r_orth_lim_-2,WF,Fromright,opt);
        }

    is_ortho_ = true;
    }
template void MPOt<ITensor>::
position(int b, const Option& opt);
template void MPOt<IQTensor>::
position(int b, const Option& opt);

template <class Tensor>
void MPOt<Tensor>::
orthogonalize(const Option& opt)
    {
    //Do a half-sweep to the right, orthogonalizing each bond
    //but do not truncate since the basis to the right might not
    //be ortho (i.e. use the current m).
    //svd_.useOrigM(true);
    int orig_maxm = svd_.maxm();
    Real orig_cutoff = svd_.cutoff();
    svd_.maxm(MAX_M);
    svd_.cutoff(MIN_CUT);

    position(N);
    //Now basis is ortho, ok to truncate
    svd_.useOrigM(false);
    svd_.maxm(orig_maxm);
    svd_.cutoff(orig_cutoff);
    position(1);

    is_ortho_ = true;
    }
template
void MPOt<ITensor>::orthogonalize(const Option& opt);
template
void MPOt<IQTensor>::orthogonalize(const Option& opt);

template <class Tensor>
void MPOt<Tensor>::
svdBond(int b, const Tensor& AA, Direction dir, const Option& opt)
    {
    if(opt == PreserveShape())
        {
        //The idea of the preserve_shape flag is to 
        //leave any external indices of the MPO on the
        //tensors they originally belong to
        Error("preserve_shape not currently implemented");
        }

    if(dir == Fromleft && b-1 > l_orth_lim_)
        {
        std::cout << boost::format("b=%d, l_orth_lim_=%d")
                %b%l_orth_lim_ << std::endl;
        Error("b-1 > l_orth_lim_");
        }
    if(dir == Fromright && b+2 < r_orth_lim_)
        {
        std::cout << boost::format("b=%d, r_orth_lim_=%d")
                %b%r_orth_lim_ << std::endl;
        Error("b+2 < r_orth_lim_");
        }

    SparseT D;
    svd_.svd(b,AA,A[b],D,A[b+1]);

    //Push singular values/amplitudes
    //to the right or left as requested
    //and update orth_lims
    if(dir == Fromleft)
        {
        A[b+1] *= D;

        l_orth_lim_ = b;
        if(r_orth_lim_ < b+2) r_orth_lim_ = b+2;
        }
    else //dir == Fromright
        {
        A[b] *= D;

        if(l_orth_lim_ > b-1) l_orth_lim_ = b-1;
        r_orth_lim_ = b+1;
        }
    }
template void MPOt<ITensor>::
svdBond(int b, const ITensor& AA, Direction dir, const Option& opt);
template void MPOt<IQTensor>::
svdBond(int b, const IQTensor& AA, Direction dir, const Option& opt);

template <class Tensor>
MPOt<Tensor>& MPOt<Tensor>::
operator+=(const MPOt<Tensor>& other_)
    {
    if(doWrite())
        Error("operator+= not supported if doWrite(true)");

    //cout << "calling new orthog in sum" << endl;
    if(!this->isOrtho())
        {
        try { 
            orthogonalize(); 
            }
        catch(const ResultIsZero& rz) 
            { 
            *this = other_;
            return *this;
            }
        }

    if(!other_.isOrtho())
        {
        MPOt<Tensor> other(other_);
        try { 
            other.orthogonalize(); 
            }
        catch(const ResultIsZero& rz) 
            { 
            return *this;
            }
        return addNoOrth(other);
        }

    return addNoOrth(other_);
    }
template
MPOt<ITensor>& MPOt<ITensor>::operator+=(const MPOt<ITensor>& other);
template
MPOt<IQTensor>& MPOt<IQTensor>::operator+=(const MPOt<IQTensor>& other);

int 
findCenter(const IQMPO& psi)
    {
    for(int j = 1; j <= psi.NN(); ++j) 
        {
        const IQTensor& A = psi.AA(j);
        if(A.r() == 0) Error("Zero rank tensor in IQMPO");
        bool allOut = true;
        for(int i = 1; i <= A.r(); ++i)
            {
            //Only look at Link IQIndices
            if(A.index(i).type() != Link) continue;

            if(A.index(i).dir() != Out)
                {
                allOut = false;
                break;
                }
            }

        //Found the ortho. center
        if(allOut) return j;
        }
    return -1;
    }

void
checkQNs(const IQMPO& psi)
    {
    const int N = psi.NN();

    QN zero;

    int center = findCenter(psi);
    //std::cerr << boost::format("Found the OC at %d\n") % center;
    if(center == -1)
        {
        Error("Did not find an ortho. center");
        }

    //Check that all IQTensors have zero div
    //including the ortho. center
    for(int i = 1; i <= N; ++i) 
        {
        if(psi.AA(i).isNull())
            {
            std::cerr << boost::format("AA(%d) null, QNs not well defined\n")%i;
            Error("QNs not well defined");
            }
        try {
            checkQNs(psi.AA(i));
            }
        catch(const ITError& e)
            {
            std::cerr << "At i = " << i << "\n";
            throw e;
            }
        if(psi.AA(i).div() != zero)
            {
            std::cerr << "At i = " << i << "\n";
            Print(psi.AA(i));
            Error("Non-zero div IQTensor in IQMPO");
            }
        }

    //Check arrows from left edge
    for(int i = 1; i < center; ++i)
        {
        if(psi.RightLinkInd(i).dir() != In) 
            {
            std::cerr << boost::format("checkQNs: At site %d to the left of the OC, Right side Link not pointing In\n")%i;
            Error("Incorrect Arrow in IQMPO");
            }
        if(i > 1)
            {
            if(psi.LeftLinkInd(i).dir() != Out) 
                {
                std::cerr << boost::format("checkQNs: At site %d to the left of the OC, Left side Link not pointing Out\n")%i;
                Error("Incorrect Arrow in IQMPO");
                }
            }
        }

    //Check arrows from right edge
    for(int i = N; i > center; --i)
        {
        if(i < N)
        if(psi.RightLinkInd(i).dir() != Out) 
            {
            std::cerr << boost::format("checkQNs: At site %d to the right of the OC, Right side Link not pointing Out\n")%i;
            Error("Incorrect Arrow in IQMPO");
            }
        if(psi.LeftLinkInd(i).dir() != In) 
            {
            std::cerr << boost::format("checkQNs: At site %d to the right of the OC, Left side Link not pointing In\n")%i;
            Error("Incorrect Arrow in IQMPO");
            }
        }
    }

template <class MPOType>
void 
nmultMPO(const MPOType& Aorig, const MPOType& Borig, MPOType& res,Real cut, int maxm)
    {
    typedef typename MPOType::TensorT Tensor;
    typedef typename MPOType::IndexT IndexT;
    if(Aorig.NN() != Borig.NN()) Error("nmultMPO(MPOType): Mismatched N");
    int N = Borig.NN();
    MPOType A(Aorig), B(Borig);

    SVDWorker svd = A.svd();
    svd.cutoff(cut);
    svd.maxm(maxm);

    A.position(1);
    B.position(1);
    B.primeall();

    res=A;
    res.primelinks(0,4);
    res.mapprime(1,2,primeSite);

    Tensor clust,nfork;
    vector<int> midsize(N);
    for(int i = 1; i < N; ++i)
        {
        if(i == 1) 
            { 
            clust = A.AA(i) * B.AA(i); 
            }
        else       
            { 
            clust = nfork * A.AA(i) * B.AA(i); 
            }

        if(i == N-1) break;

        IndexT oldmid = res.RightLinkInd(i);
        nfork = Tensor(A.RightLinkInd(i),B.RightLinkInd(i),oldmid);

        /*
        if(clust.norm() == 0) // this product gives 0 !!
            { 
            cerr << boost::format("WARNING: clust.norm()==0 in nmultMPO (i=%d).\n")%i; 
            res *= 0;
            return; 
            }
            */

        svd.denmatDecomp(i,clust, res.AAnc(i), nfork,Fromleft);

        IndexT mid = index_in_common(res.AA(i),nfork,Link);
        mid.conj();
        midsize[i] = mid.m();
        res.AAnc(i+1) = Tensor(mid,conj(res.si(i+1)),primed(res.si(i+1),2),res.RightLinkInd(i+1));
        }

    nfork = clust * A.AA(N) * B.AA(N);

    /*
    if(nfork.norm() == 0) // this product gives 0 !!
        { 
        cerr << "WARNING: nfork.norm()==0 in nmultMPO\n"; 
        res *= 0;
        return; 
        }
        */

    res.doSVD(N-1,nfork,Fromright);
    res.noprimelink();
    res.mapprime(2,1,primeSite);
    res.cutoff(cut);
    res.orthogonalize();

    }//void nmultMPO(const MPOType& Aorig, const IQMPO& Borig, IQMPO& res,Real cut, int maxm)
template
void nmultMPO(const MPO& Aorig, const MPO& Borig, MPO& res,Real cut, int maxm);
template
void nmultMPO(const IQMPO& Aorig, const IQMPO& Borig, IQMPO& res,Real cut, int maxm);


template <class Tensor>
void 
zipUpApplyMPO(const MPSt<Tensor>& psi, const MPOt<Tensor>& K, MPSt<Tensor>& res, Real cutoff, int maxm)
    {
    typedef typename Tensor::IndexT
    IndexT;

    if(&psi == &res)
        Error("psi and res must be different MPS instances");

    if(cutoff < 0) cutoff = psi.cutoff();
    if(maxm < 0) maxm = psi.maxm();

    const int N = psi.NN();
    if(K.NN() != N) 
        Error("Mismatched N in napplyMPO");

    if(!psi.isOrtho() || psi.orthoCenter() != 1)
        Error("Ortho center of psi must be site 1");

    if(!K.isOrtho() || K.orthoCenter() != 1)
        Error("Ortho center of K must be site 1");

    SVDWorker svd = K.svd();
    svd.cutoff(cutoff);
    svd.maxm(maxm);

    res = psi; 
    res.maxm(maxm); 
    res.cutoff(cutoff);
    res.primelinks(0,4);
    res.mapprime(0,1,primeSite);

    Tensor clust,nfork;
    vector<int> midsize(N);
    int maxdim = 1;
    for(int i = 1; i < N; i++)
        {
        if(i == 1) { clust = psi.AA(i) * K.AA(i); }
        else { clust = nfork * (psi.AA(i) * K.AA(i)); }
        if(i == N-1) break; //No need to SVD for i == N-1

        IndexT oldmid = res.RightLinkInd(i); assert(oldmid.dir() == Out);
        nfork = Tensor(psi.RightLinkInd(i),K.RightLinkInd(i),oldmid);
        //if(clust.iten_size() == 0)	// this product gives 0 !!
	    //throw ResultIsZero("clust.iten size == 0");
        svd.denmatDecomp(i,clust, res.AAnc(i), nfork,Fromleft);
        IndexT mid = index_in_common(res.AA(i),nfork,Link);
        //assert(mid.dir() == In);
        mid.conj();
        midsize[i] = mid.m();
        maxdim = max(midsize[i],maxdim);
        assert(res.RightLinkInd(i+1).dir() == Out);
        res.AAnc(i+1) = Tensor(mid,primed(res.si(i+1)),res.RightLinkInd(i+1));
        }
    nfork = clust * psi.AA(N) * K.AA(N);
    //if(nfork.iten_size() == 0)	// this product gives 0 !!
	//throw ResultIsZero("nfork.iten size == 0");

    res.doSVD(N-1,nfork,Fromright);
    res.noprimelink();
    res.mapprime(1,0,primeSite);
    res.position(1);
    res.maxm(psi.maxm()); 
    res.cutoff(psi.cutoff());
    } //void zipUpApplyMPO
template
void 
zipUpApplyMPO(const MPS& x, const MPO& K, MPS& res, Real cutoff, int maxm);
template
void 
zipUpApplyMPO(const IQMPS& x, const IQMPO& K, IQMPS& res, Real cutoff, int maxm);

//Expensive: scales as m^3 k^3!
template<class Tensor>
void 
exactApplyMPO(const MPSt<Tensor>& x, const MPOt<Tensor>& K, MPSt<Tensor>& res)
    {
    typedef typename Tensor::IndexT
    IndexT;
    typedef typename Tensor::CombinerT
    CombinerT;

    int N = x.NN();
    if(K.NN() != N) Error("Mismatched N in exactApplyMPO");

    if(&res != &x)
        res = x;

    res.AAnc(1) = x.AA(1) * K.AA(1);
    for(int j = 1; j < N; ++j)
        {
        //cerr << boost::format("exact_applyMPO: step %d\n") % j;
        //Compute product of MPS tensor and MPO tensor
        res.AAnc(j+1) = x.AA(j+1) * K.AA(j+1); //m^2 k^2 d^2

        //Add common IQIndices to IQCombiner
        CombinerT comb; 
        comb.doCondense(false);
        for(int ii = 1; ii <= res.AA(j).r(); ++ii)
            {
            const IndexT& I = res.AA(j).index(ii);
            if(res.AA(j+1).hasindex(I) && I != IndexT::IndReIm())
                comb.addleft(I);
            }
        comb.init(nameint("a",j));

        //Apply combiner to product tensors
        res.AAnc(j) = res.AA(j) * comb; //m^3 k^3 d
        res.AAnc(j+1) = conj(comb) * res.AA(j+1); //m^3 k^3 d
        }
    res.mapprime(1,0,primeSite);
    //res.orthogonalize();
    } //void exact_applyMPO
template
void 
exactApplyMPO(const MPS& x, const MPO& K, MPS& res);
template
void 
exactApplyMPO(const IQMPS& x, const IQMPO& K, IQMPS& res);


template<class Tensor>
void 
expsmallH(const MPOt<Tensor>& H, MPOt<Tensor>& K, 
          Real tau, Real Etot, Real Kcutoff)
    {
    const int maxm = 400;

    HamBuilder hb(H.model());

    MPOt<Tensor> Hshift;
    hb.getMPO(Hshift,-Etot);
    Hshift += H;
    Hshift.AAnc(1) *= -tau;

    vector<MPOt<Tensor> > xx(2);
    hb.getMPO(xx.at(0),1.0);
    xx.at(1) = Hshift;

    //
    // Exponentiate by building up a Taylor series in reverse:
    //      o=1    o=2      o=3      o=4  
    // K = 1-t*H*(1-t*H/2*(1-t*H/3*(1-t*H/4*(...))))
    //
    for(int o = 50; o >= 1; --o)
        {
        if(o > 1) xx[1].AAnc(1) *= 1.0 / o;

        Real errlim = 1E-14;

        sum(xx,K,errlim,maxm);
        if(o > 1)
            nmultMPO(K,Hshift,xx[1],errlim,maxm);
        }
    }
template
void 
expsmallH(const MPO& H, MPO& K, Real tau, Real Etot, Real Kcutoff);
template
void 
expsmallH(const IQMPO& H, IQMPO& K, Real tau, Real Etot, Real Kcutoff);

template<class Tensor>
void 
expH(const MPOt<Tensor>& H, MPOt<Tensor>& K, Real tau, Real Etot,
     Real Kcutoff, int ndoub)
    {
    Real ttau = tau / pow(2.0,ndoub);
    //cout << "ttau in expH is " << ttau << endl;

    K.cutoff(0.1 * Kcutoff * pow(0.25,ndoub));
    expsmallH(H, K, ttau,Etot,K.cutoff());

    cout << "Starting doubling in expH" << endl;
    for(int doub = 1; doub <= ndoub; ++doub)
        {
        //cout << " Double step " << doub << endl;
        if(doub == ndoub) 
            K.cutoff(Kcutoff);
        else
            K.cutoff(0.1 * Kcutoff * pow(0.25,ndoub-doub));
        //cout << "in expH, K.cutoff is " << K.cutoff << endl;
        MPOt<Tensor> KK;
        nmultMPO(K,K,KK,K.cutoff(),K.maxm());
        K = KK;
        /*
        if(doub == ndoub)
            {
            cout << "step " << doub << ", K is " << endl;
            cout << "K.cutoff, K.maxm are " << K.cutoff SP K.maxm << endl;
            for(int i = 1; i <= N; i++)
                cout << i SP K.A[i];
            }
        */
        }
    }
template
void 
expH(const MPO& H, MPO& K, Real tau, Real Etot,Real Kcutoff, int ndoub);
template
void 
expH(const IQMPO& H, IQMPO& K, Real tau, Real Etot,Real Kcutoff, int ndoub);

