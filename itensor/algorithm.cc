#include "algorithm.h"


//Orthogonalizing DMRG. Puts in an energy penalty if psi has an overlap with any MPS in 'other'.
Real dmrg(MPS& psi, const MPO& finalham, const Sweeps& sweeps, const vector<MPS>& other, DMRGOpts& opts)
{
    int debuglevel = 1;
    if(opts.quiet) debuglevel = 0;

    Real energy = 0.0, last_energy = -10000;

    int N = psi.NN();

    psi.position(1);

    if(finalham.is_complex() && !psi.is_complex())
    {
        for(int i = 1; i <= N; ++i) psi.AAnc(i) = psi.AA(i)*Complex_1;
    }

    vector<ITensor> leftright(N+1);
    vector< vector<ITensor> > lrother(other.size());
    MPS psiconj(psi);
    for(int i = 1; i <= finalham.NN(); i++)
	{
        psiconj.AAnc(i) = conj(psi.AA(i)); 
        psiconj.AAnc(i).doprime(primeBoth);
	}

    leftright[N-1] = psi.AA(N) * finalham.AA(N) * psiconj.AA(N);

    for(int l = N-2; l >= 2; l--)
	leftright[l] = leftright[l+1] * psi.AA(l+1) * finalham.AA(l+1) * 
		    psiconj.AA(l+1);

    for(unsigned int o = 0; o < other.size(); o++)
	{
        lrother[o].resize(N);
        lrother[o][N-1] = conj(psi.AA(N))* other[o].AA(N);
        for(int l = N-2; l >= 2; l--)
            lrother[o][l] = lrother[o][l+1] * conj(psi.AA(l+1)) * other[o].AA(l+1);
	}

    for(int sw = 1; sw <= sweeps.nsweep(); sw++)
    {
        int largest_m = -1;
        int max_eigs_bond = -1;
        Vector max_eigs(1); max_eigs = 2; //max in the sense of slowly decaying
        for(int l = 1, ha = 1; ha != 3; sweepnext(l,ha,N))
        {
            if(!opts.quiet) cout << format("Sweep=%d, HS=%d, Bond=(%d,%d)\n") % sw % ha % l % (l+1);

            ITensor mpoh = finalham.AA(l) * finalham.AA(l+1);

            ITensor phi = psi.AA(l) * psi.AA(l+1);

            int dim = phi.dat().Length();
            Matrix evecs(sweeps.niter(sw),dim);
            Vector evals;
            phi.AssignToVec(evecs.Row(1));
            evecs.Row(1) *= 1.0 / Norm(evecs.Row(1));

            //printdat = false; cerr << "Multiple state phi = " << phi << "\n"; 

            LocalHamOrth<ITensor> lham(leftright[l],leftright[l+1],mpoh,phi,opts.orth_weight);
            lham.other.resize(other.size());
            if(l == 1)
            {
                for(unsigned int o = 0; o < other.size(); o++)
                    lham.other[o] = other[o].AA(l) * other[o].AA(l+1) * lrother[o][l+1];
            }
            else if(l == N-1)
            {
                for(unsigned int o = 0; o < other.size(); o++)
                    lham.other[o] = lrother[o][l] * other[o].AA(l) * other[o].AA(l+1);
            }
            else 
            {
                for(unsigned int o = 0; o < other.size(); o++)
                    lham.other[o] = lrother[o][l] * other[o].AA(l) * other[o].AA(l+1) * lrother[o][l+1];
            }
            David(lham,1,1e-4,evals,evecs,1,1,debuglevel);

            energy = evals(1);
            phi.AssignFromVec(evecs.Row(1));

            do_denmat_Real(phi,psi.AAnc(l),psi.AAnc(l+1),sweeps.cutoff(sw),sweeps.minm(sw),sweeps.maxm(sw),(ha==1 ? Fromleft : Fromright));

            psiconj.AAnc(l) = conj(psi.AA(l)); psiconj.AAnc(l).doprime(primeBoth);
            psiconj.AAnc(l+1) = conj(psi.AA(l+1)); psiconj.AAnc(l+1).doprime(primeBoth);

            Index ll = psi.LinkInd(l);
            if(!opts.quiet) 
            { cout << format("    Truncated to Cutoff=%.1E, Max_m=%d, m=%d\n") % sweeps.cutoff(sw) % sweeps.maxm(sw) % ll.m(); }

            //Keep track of the largest_m, slowest decaying denmat eigs
            if(opts.printeigs)
            {
                largest_m = max(largest_m,ll.m());
                //if(deigs.Length() >= max(largest_m,max_eigs.Length()) && max_eigs(max_eigs.Length()) < deigs(max_eigs.Length())) 
                if(lastd(1) < max_eigs(1) && l != 1 && l != (N-1)) { max_eigs = lastd; max_eigs_bond = l; }

                if(l == 1 && ha == 2) 
                {
                    cout << "\n    Largest m during sweep " << sw << " was " << largest_m << "\n";
                    cout << format("    Eigs at bond %d: ") % max_eigs_bond;
                    for(int j = 1; j <= min(max_eigs.Length(),10); ++j) 
                    {
                        cout << format(max_eigs(j) > 1E-2 ? ("%.2f") : ("%.2E")) % max_eigs(j);
                        cout << ((j != min(max_eigs.Length(),10)) ? ", " : "\n");
                    }

                    cout << format("    Energy after sweep %d is %f\n") % sw % energy;
                }
            }

            if(ha == 1)
            {
                if(l == 1)
                {
                    leftright[2] = psi.AA(1) * finalham.AA(1) * psiconj.AA(1);
                    for(unsigned int o = 0; o < other.size(); o++)
                        lrother[o][2] = conj(psi.AA(1)) * other[o].AA(1);
                }
                else if(l != N-1)
                {
                    leftright[l+1] = leftright[l] * psi.AA(l) * finalham.AA(l) * psiconj.AA(l);
                    for(unsigned int o = 0; o < other.size(); o++)
                        lrother[o][l+1] = lrother[o][l] * conj(psi.AA(l)) * other[o].AA(l);
                }
            }
            else
            {
            if(l == N-1)
            {
                leftright[l] = psi.AA(N) * finalham.AA(N) * psiconj.AA(N);
                for(unsigned int o = 0; o < other.size(); o++)
                    lrother[o][l] = conj(psi.AA(N)) * other[o].AA(N);
            }
            else if(l != 1)
            {
                leftright[l] = leftright[l+1] * psi.AA(l+1) * finalham.AA(l+1) * 
                    psiconj.AA(l+1);
                for(unsigned int o = 0; o < other.size(); o++)
                    lrother[o][l] = lrother[o][l+1] * conj(psi.AA(l+1)) * other[o].AA(l+1);
            }
            }

        }

        if(opts.energy_errgoal > 0 && sw%2 == 0)
        {
            Real dE = fabs(energy-last_energy);
            if(dE < opts.energy_errgoal)
            {
                cout << format("    Energy error goal met (dE = %E); returning after %d sweeps.\n") % dE % sw;
                return energy;
            }
        }
        last_energy = energy;
    }
    return energy;
}

/* Deprecated, use MPOSet to work with a set of MPOs
Real dmrg(MPS& psi, const vector<MPO>& H, const Sweeps& sweeps, DMRGOpts& opts)
{
    int debuglevel = 1;
    if(opts.quiet) debuglevel = 0;
    Real energy, last_energy = -10000;

    const int N = psi.NN();
    const int NH = H.size();

    psi.position(1);

    if(H[0].is_complex() && !psi.is_complex())
    {
        for(int i = 1; i <= N; ++i) psi.AAnc(i) = psi.AA(i)*Complex_1;
    }

    MPS psiconj(psi);
    for(int i = 1; i <= H[0].NN(); i++)
	{
        psiconj.AAnc(i) = conj(psi.AA(i));
        psiconj.AAnc(i).doprime(primeBoth);
	}

    vector< vector<ITensor> > leftright(N+1);
    for(int j = 0; j < N; ++j) leftright[j].resize(NH);

    for(int n = 0; n < NH; ++n)
    {
        leftright[N-1][n] = psi.AA(N) * H[n].AA(N) * psiconj.AA(N);

        for(int l = N-2; l >= 2; l--)
            leftright[l][n] = leftright[l+1][n] * psi.AA(l+1) * H[n].AA(l+1) * psiconj.AA(l+1);
    }

    vector<ITensor> mpoh(NH);

    for(int sw = 1; sw <= sweeps.nsweep(); sw++)
    {
        int largest_m = -1;
        int max_eigs_bond = -1;
        Vector max_eigs(1); max_eigs = 2; //max in the sense of slowly decaying
        Vector center_eigs(1); center_eigs = 2;
        for(int l = 1, ha = 1; ha != 3; sweepnext(l,ha,N))
        {
            if(!opts.quiet) cout << format("Sweep=%d, HS=%d, Bond=(%d,%d)\n") % sw % ha % l % (l+1);

            for(int n = 0; n < NH; ++n) 
            {
                mpoh[n] = H[n].AA(l) * H[n].AA(l+1);
            }

            ITensor phi = psi.AA(l) * psi.AA(l+1);

            energy = doDavidson(phi,mpoh,leftright[l],leftright[l+1],sweeps.niter(sw),debuglevel,1e-4);

            do_denmat_Real(phi,psi.AAnc(l),psi.AAnc(l+1),sweeps.cutoff(sw),sweeps.minm(sw),sweeps.maxm(sw),(ha==1 ? Fromleft : Fromright));

            psiconj.AAnc(l) = conj(psi.AA(l)); psiconj.AAnc(l).doprime(primeBoth);
            psiconj.AAnc(l+1) = conj(psi.AA(l+1)); psiconj.AAnc(l+1).doprime(primeBoth);

            Index ll = psi.LinkInd(l);
            if(!opts.quiet) 
            { cout << format("    Truncated to Cutoff=%.1E, Max_m=%d, m=%d\n") % sweeps.cutoff(sw) % sweeps.maxm(sw) % ll.m(); }

            //Keep track of the largest_m, slowest decaying denmat eigs
            if(opts.printeigs)
            {
                largest_m = max(largest_m,ll.m());
                if(lastd(1) < max_eigs(1) && l != 1 && l != (N-1)) { max_eigs = lastd; max_eigs_bond = l; }
                if(l == psi.NN()/2) 
                {
                    center_eigs = lastd;
                    opts.bulk_entanglement_gap = (lastd.Length() >= 2 ? lastd(1)-lastd(2) : 1);
                }

                if(l == 1 && ha == 2) 
                {
                    cout << "\n    Largest m during sweep " << sw << " was " << largest_m << "\n";
                    cout << format("    Eigs at bond %d: ") % max_eigs_bond;
                    for(int j = 1; j <= min(max_eigs.Length(),10); ++j) 
                    {
                        cout << format(max_eigs(j) > 1E-2 ? ("%.2f") : ("%.2E")) % max_eigs(j);
                        cout << ((j != min(max_eigs.Length(),10)) ? ", " : "\n");
                    }
                    cout << "    Eigs at center bond: ";
                    for(int j = 1; j <= min(center_eigs.Length(),10); ++j) 
                    {
                        cout << format(center_eigs(j) > 1E-2 ? ("%.2f") : ("%.2E")) % center_eigs(j);
                        cout << ((j != min(center_eigs.Length(),10)) ? ", " : "\n");
                    }
                    cout << format("    Bulk entanglement gap = %f\n") % opts.bulk_entanglement_gap;

                    cout << format("    Energy after sweep %d is %f\n") % sw % energy;
                    for(int n = 1; n < NH; ++n)
                    {
                      Real re,im; psiHphi(psi,H[n],psi,re,im);
                      cout << format("    Expectation value of Op %d = %f\n") % n % re;
                    }
                }
            }

            if(ha == 1)
            {
                for(int n = 0; n < NH; ++n)
                {
                if(l == 1)
                    leftright[2][n] = psi.AA(1) * H[n].AA(1) * psiconj.AA(1);
                else if(l != N-1)
                    leftright[l+1][n] = leftright[l][n] * psi.AA(l) * H[n].AA(l) * psiconj.AA(l);
                }
            }
            else
            {
                for(int n = 0; n < NH; ++n)
                {
                if(l == N-1)
                    leftright[l][n] = psi.AA(N) * H[n].AA(N) * psiconj.AA(N);
                else if(l != 1)
                    leftright[l][n] = leftright[l+1][n] * psi.AA(l+1) * H[n].AA(l+1) * psiconj.AA(l+1);
                }
            }


        } //for loop over l

        if(opts.energy_errgoal > 0 && sw%2 == 0)
        {
            Real dE = fabs(energy-last_energy);
            if(dE < opts.energy_errgoal)
            {
                cout << format("    Energy error goal met (dE = %E); returning after %d sweeps.\n") % dE % sw;
                return energy;
            }
        }
        last_energy = energy;

    } //for loop over sw

    return energy;
}
*/

Real ucdmrg(MPS& psi, const ITensor& LB, const ITensor& RB, const MPO& H, const Sweeps& sweeps, DMRGOpts& opts, bool preserve_edgelink)
{
    const bool useleft = (LB.r() != 0);
    const bool useright = (RB.r() != 0);

    int debuglevel = 1;
    if(opts.quiet) debuglevel = 0;
    Real energy, last_energy = -10000;

    int N = psi.NN();

    psi.position(1,preserve_edgelink);

    if(H.is_complex()) psi.AAnc(1) *= Complex_1;

    MPS psiconj(psi);
    for(int i = 1; i <= psi.NN(); i++)
	{
        psiconj.AAnc(i) = conj(psi.AA(i));
        psiconj.AAnc(i).doprime(primeBoth);
	}

    vector<ITensor> leftright(N);
    if(useright) leftright[N-1] = RB * psi.AA(N); 
    else         leftright[N-1] = psi.AA(N);
    leftright[N-1] *= H.AA(N);
    leftright[N-1] *= psiconj.AA(N);

    for(int l = N-2; l >= 2; l--)
    {
        leftright[l] = leftright[l+1]; 
        leftright[l] *= psi.AA(l+1);
        leftright[l] *= H.AA(l+1);
        leftright[l] *= psiconj.AA(l+1);
    }

    for(int sw = 1; sw <= sweeps.nsweep(); sw++)
    {
        int largest_m = -1;
        int max_eigs_bond = -1;
        Vector max_eigs(1); max_eigs = 2; //max in the sense of slowly decaying
        Vector center_eigs(1); center_eigs = 2;
        for(int l = 1, ha = 1; ha != 3; sweepnext(l,ha,N))
        {
            if(!opts.quiet) cout << format("Sweep=%d, HS=%d, Bond=(%d,%d)\n") % sw % ha % l % (l+1);

            ITensor mpoh = H.AA(l) * H.AA(l+1);

            ITensor phi = psi.AA(l) * psi.AA(l+1);

            energy = doDavidson(phi,mpoh,leftright[l],leftright[l+1],sweeps.niter(sw),debuglevel,1e-4);

            /*
            if(preserve_edgelink)
            if((l == 1 && useleft) || (l == (psi.NN()-1) && useright))
            {
                const ITensor& B = (l == 1 ? LB : RB);
                const int s = (l==1 ? 1 : psi.NN());
                ITensor newA(psi.AA(s).findtype(Site),index_in_common(psi.AA(s),B,Link));
            }
            */

            do_denmat_Real(phi,psi.AAnc(l),psi.AAnc(l+1),sweeps.cutoff(sw),sweeps.minm(sw),sweeps.maxm(sw),(ha==1 ? Fromleft : Fromright));

            psiconj.AAnc(l) = conj(psi.AA(l)); psiconj.AAnc(l).doprime(primeBoth);
            psiconj.AAnc(l+1) = conj(psi.AA(l+1)); psiconj.AAnc(l+1).doprime(primeBoth);

            Index ll = psi.LinkInd(l);
            if(!opts.quiet) 
            { cout << format("    Truncated to Cutoff=%.1E, Max_m=%d, m=%d\n") % sweeps.cutoff(sw) % sweeps.maxm(sw) % ll.m(); }

            //Keep track of the largest_m, slowest decaying denmat eigs
            if(opts.printeigs)
            {
                largest_m = max(largest_m,ll.m());
                if(lastd(1) < max_eigs(1) && l != 1 && l != (N-1)) { max_eigs = lastd; max_eigs_bond = l; }
                if(l == psi.NN()/2) 
                {
                    center_eigs = lastd;
                    opts.bulk_entanglement_gap = (lastd.Length() >= 2 ? lastd(1)-lastd(2) : 1);
                }

                if(l == 1 && ha == 2) 
                {
                    cout << "\n    Largest m during sweep " << sw << " was " << largest_m << "\n";
                    cout << format("    Eigs at bond %d: ") % max_eigs_bond;
                    for(int j = 1; j <= min(max_eigs.Length(),10); ++j) 
                    {
                        cout << format(max_eigs(j) > 1E-2 ? ("%.2f") : ("%.2E")) % max_eigs(j);
                        cout << ((j != min(max_eigs.Length(),10)) ? ", " : "\n");
                    }
                    cout << "    Eigs at center bond: ";
                    for(int j = 1; j <= min(center_eigs.Length(),10); ++j) 
                    {
                        cout << format(center_eigs(j) > 1E-2 ? ("%.2f") : ("%.2E")) % center_eigs(j);
                        cout << ((j != min(center_eigs.Length(),10)) ? ", " : "\n");
                    }
                    cout << format("    Bulk entanglement gap = %f\n") % opts.bulk_entanglement_gap;

                    cout << format("    Energy after sweep %d is %f\n") % sw % energy;
                }
            }

            if(ha == 1)
            {
                if(l == 1)
                {
                    if(useleft) leftright[2] = LB * psi.AA(1); 
                    else        leftright[2] = psi.AA(1); 
                    leftright[2] *= H.AA(1);
                    leftright[2] *= psiconj.AA(1);
                }
                else if(l != N-1)
                {
                    leftright[l+1] = leftright[l]; 
                    leftright[l+1] *= psi.AA(l); 
                    leftright[l+1] *= H.AA(l); 
                    leftright[l+1] *= psiconj.AA(l);
                }
            }
            else
            {
                if(l == N-1)
                {
                    if(useright) leftright[l] = RB * psi.AA(N); 
                    else         leftright[l] = psi.AA(N); 
                    leftright[l] *= H.AA(N);
                    leftright[l] *= psiconj.AA(N);
                }
                else if(l != 1)
                {
                    leftright[l] = leftright[l+1];
                    leftright[l] *= psi.AA(l+1);
                    leftright[l] *= H.AA(l+1);
                    leftright[l] *= psiconj.AA(l+1);
                }
            }


        } //for loop over l

        if(opts.energy_errgoal > 0 && sw%2 == 0)
        {
            Real dE = fabs(energy-last_energy);
            if(dE < opts.energy_errgoal)
            {
                cout << format("    Energy error goal met (dE = %E); returning after %d sweeps.\n") % dE % sw;
                return energy;
            }
        }
        last_energy = energy;

    } //for loop over sw

    return energy;
}

void nmultMPO(const IQMPO& Aorig, const IQMPO& Borig, IQMPO& res,Real cut, int maxm)
{
    if(Aorig.NN() != Borig.NN()) Error("nmultMPO(IQMPO): Mismatched N");
    int N = Borig.NN();
    IQMPO A(Aorig), B(Borig);

    A.position(1);
    B.position(1);
    B.primeall();

    res=A;
    res.primelinks(0,4);
    res.mapprime(1,2,primeSite);

    IQTensor clust,nfork;
    vector<int> midsize(N);
    for(int i = 1; i < N; ++i)
	{
        if(i == 1) { clust = A.AA(i) * B.AA(i); }
        else       { clust = nfork * A.AA(i) * B.AA(i); }
        if(i == N-1) break;

        IQIndex oldmid = res.RightLinkInd(i);
        nfork = IQTensor(A.RightLinkInd(i),B.RightLinkInd(i),oldmid);
        if(clust.iten_size() == 0)	// this product gives 0 !!
        { cerr << format("WARNING: clust.iten_size()==0 in nmultMPO (i=%d).\n")%i; res = IQMPO(); return; }
        tensorSVD(clust, res.AAnc(i), nfork,cut,1,maxm,Fromleft);
        IQIndex mid = index_in_common(res.AA(i),nfork,Link);
        assert(mid.dir() == In);
        mid.conj();
        midsize[i] = mid.m();
        assert(res.RightLinkInd(i+1).dir() == Out);
        assert(res.si(i+1).dir() == Out);
        res.AAnc(i+1) = IQTensor(mid,conj(res.si(i+1)),res.si(i+1).primed().primed(),res.RightLinkInd(i+1));
	}

    nfork = clust * A.AA(N) * B.AA(N);
    if(nfork.iten_size() == 0)	// this product gives 0 !!
    { cerr << "WARNING: nfork.iten_size()==0 in nmultMPO\n"; res = IQMPO(); return; }

    res.doSVD(N-1,nfork,Fromright,false);
    res.noprimelink();
    res.mapprime(2,1,primeSite);
    res.cutoff = cut;
    res.position(N);
    res.position(1);

}//void nmultMPO(const IQMPO& Aorig, const IQMPO& Borig, IQMPO& res,Real cut, int maxm)

void psiHKphi(const IQMPS& psi, const IQMPO& H, const IQMPO& K,const IQMPS& phi, Real& re, Real& im) //<psi|H K|phi>
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
    L = ((((L * phi.AA(N)) * H.AA(N)) * Kp.AA(N)) * psiconj.AA(N)) * IQTSing;
    //cout << "in psiHKpsi, L is "; PrintDat(L);
    L.GetSingComplex(re,im);
}
Real psiHKphi(const IQMPS& psi, const IQMPO& H, const IQMPO& K,const IQMPS& phi) //<psi|H K|phi>
{
    Real re,im;
    psiHKphi(psi,H,K,phi,re,im);
    if(fabs(im) > 1E-12) Error("Non-zero imaginary part in psiHKphi");
    return re;
}

void napplyMPO(const IQMPS& x, const IQMPO& K, IQMPS& res, Real cutoff, int maxm)
{
    if(cutoff < 0) cutoff = x.cutoff;
    if(maxm < 0) maxm = x.maxm;
    int N = x.NN();
    if(K.NN() != N) Error("Mismatched N in napplyMPO");
    if(x.right_lim() > 3)
    {
        cerr << "x is " << endl << x << endl;
        Error("bad right_lim for x");
    }
    if(K.right_lim() > 3)
    {
        //cerr << "K is " << endl << K << endl;
        Error("bad right_lim for K");
    }

    res = x; res.maxm = maxm; res.cutoff = cutoff;
    res.primelinks(0,4);
    res.mapprime(0,1,primeSite);

    IQTensor clust,nfork;
    vector<int> midsize(N);
    int maxdim = 1;
    for(int i = 1; i < N; i++)
	{
        if(i == 1) { clust = x.AA(i) * K.AA(i); }
        else { clust = nfork * (x.AA(i) * K.AA(i)); }
        if(i == N-1) break; //No need to SVD for i == N-1

        IQIndex oldmid = res.RightLinkInd(i); assert(oldmid.dir() == Out);
        nfork = IQTensor(x.RightLinkInd(i),K.RightLinkInd(i),oldmid);
        if(clust.iten_size() == 0)	// this product gives 0 !!
        { res = IQMPS(); return; }
        tensorSVD(clust, res.AAnc(i), nfork,cutoff,1,maxm,Fromleft);
        IQIndex mid = index_in_common(res.AA(i),nfork,Link);
        assert(mid.dir() == In);
        mid.conj();
        midsize[i] = mid.m();
        maxdim = max(midsize[i],maxdim);
        assert(res.RightLinkInd(i+1).dir() == Out);
        res.AAnc(i+1) = IQTensor(mid,res.si(i+1).primed(),res.RightLinkInd(i+1));
	}
    nfork = clust * x.AA(N) * K.AA(N);
    if(nfork.iten_size() == 0)	// this product gives 0 !!
	{ res = IQMPS(); return; }

    res.doSVD(N-1,nfork,Fromright,false);
    res.noprimelink();
    res.mapprime(1,0,primeSite);
    res.position(1);
    res.maxm = x.maxm; res.cutoff = x.cutoff;

} //void napplyMPO

//Expensive: scales as m^3 k^3!
void exact_applyMPO(const IQMPS& x, const IQMPO& K, IQMPS& res)
{
    int N = x.NN();
    if(K.NN() != N) Error("Mismatched N in exact_applyMPO");

    res = x;
    res.position(1);

    res.AAnc(1) = x.AA(1) * K.AA(1);
    for(int j = 1; j < N; ++j)
	{
        //cerr << format("exact_applyMPO: step %d\n") % j;
        //Compute product of MPS tensor and MPO tensor
        res.AAnc(j+1) = x.AA(j+1) * K.AA(j+1); //m^2 k^2 d^2

        //Add common IQIndices to IQCombiner
        IQCombiner comb; comb.doCondense(false);
        foreach(const IQIndex& I, res.AA(j).iqinds())
        if(res.AA(j+1).hasindex(I) && I != IQIndReIm && I.type() != Virtual)
        { assert(I.dir() == Out); comb.addleft(I);}
        comb.init(nameint("a",j));

        //Apply combiner to product tensors
        res.AAnc(j) = res.AA(j) * comb; //m^3 k^3 d
        res.AAnc(j+1) = conj(comb) * res.AA(j+1); //m^3 k^3 d
	}
    res.mapprime(1,0,primeSite);
    //res.position(1);
} //void exact_applyMPO

//Computes an MPS which has the same overlap with psi_basis as psi_to_fit,
//but which differs from psi_basis only on the first site, and has same index
//structure as psi_basis. Result is stored to psi_to_fit on return.
void fitWF(const IQMPS& psi_basis, IQMPS& psi_to_fit)
{
    if(!psi_basis.is_ortho()) Error("fitWF: psi_basis must be orthogonolized.");
    if(psi_basis.ortho_center() != 1) Error("fitWF: psi_basis must be orthogonolized to site 1.");
    psi_to_fit.position(1);

    const IQMPS& psib = psi_basis;
    IQMPS& psif = psi_to_fit;
    IQMPS res = psib;

    int N = psib.NN();
    if(psif.NN() != N) Error("fitWF: sizes of wavefunctions must match.");

    IQTensor A = psif.AA(N) * conj(psib.AA(N));
    for(int n = N-1; n > 1; --n)
    {
        A = conj(psib.AA(n)) * A;
        A = psif.AA(n) * A;
    }
    A = psif.AA(1) * A;

    res.AAnc(1) = A;

    assert(check_QNs(res));

    psi_to_fit = res;
}
