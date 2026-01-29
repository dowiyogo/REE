#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include <iostream>
#include <vector>
#include <cmath>

// Helper rápido
double GetCounts(const char* fname) {
    TFile* f = TFile::Open(fname);
    if(!f || f->IsZombie()) return 0;
    TTree* t = (TTree*)f->Get("Scoring");
    double cnt = 0;
    if(t) {
        // Proyección rápida para debug
        TH1D* h = new TH1D("hDebug", "debug", 100, 0, 3.0);
        t->Draw("Energy>>hDebug", "", "goff");
        cnt = h->Integral();
        delete h;
    }
    f->Close();
    return cnt;
}

void debug_transmision() {
    std::vector<const char*> filesAm = {
        "Am241_0p0_REE.root", "Am241_0p01_REE.root", "Am241_0p02_REE.root", 
        "Am241_0p03_REE.root", "Am241_0p04_REE.root", "Am241_0p05_REE.root"
    };
    std::vector<const char*> filesNa = {
        "Na22_0p0_REE.root", "Na22_0p01_REE.root", "Na22_0p02_REE.root", 
        "Na22_0p03_REE.root", "Na22_0p04_REE.root", "Na22_0p05_REE.root" 
    };
    
    double I0_Am = GetCounts(filesAm[0]);
    double I0_Na = GetCounts(filesNa[0]);

    std::cout << "DEBUG RAPIDO (Leyendo TTrees)" << std::endl;
    std::cout << "Conc(%) \t I_Am \t\t T_Am \t\t I_Na \t\t T_Na \t\t R_calc" << std::endl;
    std::cout << "------------------------------------------------------------------------" << std::endl;

    for(int i=0; i<6; i++) {
        double I_Am = GetCounts(filesAm[i]);
        double I_Na = GetCounts(filesNa[i]);
        
        double T_Am = (I0_Am>0) ? I_Am/I0_Am : 0;
        double T_Na = (I0_Na>0) ? I_Na/I0_Na : 0;
        
        double L_Am = (T_Am>0) ? -log(T_Am) : 0;
        double L_Na = (T_Na>0) ? -log(T_Na) : 0;
        double R = (L_Na>0.001) ? L_Am/L_Na : 0;

        std::cout << i << ".0 \t\t " << (int)I_Am << " \t " << T_Am << " \t " 
                  << (int)I_Na << " \t " << T_Na << " \t " << R << std::endl;
    }
}