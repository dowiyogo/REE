#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TStyle.h"
#include <iostream>
#include <vector>
#include <cmath>

// Configuración de ROIs (Region of Interest) en MeV
// Ajustado para GEANT4 standard (MeV)
// Am-241 (59.5 keV) -> 0.0595 MeV
const double E_Am_Min = 0.052; 
const double E_Am_Max = 0.068; 
// Na-22 (511 keV) -> 0.511 MeV
const double E_Na_Min = 0.490;
const double E_Na_Max = 0.532; 

struct Medicion { double cuentas; double error; };

// Función adaptada para TTree
Medicion IntegrarPico(const char* filename, double eMin, double eMax) {
    TFile* f = TFile::Open(filename);
    Medicion m = {0, 0};
    
    if(!f || f->IsZombie()) {
        std::cerr << "Error abriendo: " << filename << std::endl;
        return m;
    }

    TTree* t = (TTree*)f->Get("Scoring");
    if(t) {
        // Creamos histograma temp con suficiente resolución
        // 4096 bins en 3 MeV da ~0.7 keV por bin, suficiente para LaBr3
        TH1D* h = new TH1D("hTemp", "Energy", 4096, 0, 3.0);
        t->Draw("Energy>>hTemp", "", "goff");
        
        // Convertimos energía a bins del histograma
        int binMin = h->FindBin(eMin);
        int binMax = h->FindBin(eMax);
        
        double err;
        m.cuentas = h->IntegralAndError(binMin, binMax, err);
        m.error = err;
        
        delete h;
    }
    f->Close();
    return m;
}

void analisis_LaBr3_ROI() {
    const int n = 6;
    double conc[n] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}; // %
    
    std::vector<const char*> filesAm = {
        "Am241_0p0_REE.root", "Am241_0p01_REE.root", "Am241_0p02_REE.root", 
        "Am241_0p03_REE.root", "Am241_0p04_REE.root", "Am241_0p05_REE.root"
    };
    std::vector<const char*> filesNa = {
        "Na22_0p0_REE.root", "Na22_0p01_REE.root", "Na22_0p02_REE.root", 
        "Na22_0p03_REE.root", "Na22_0p04_REE.root", "Na22_0p05_REE.root" 
    };

    std::vector<double> R_val(n), R_err(n), x_err(n, 0.0);
    Medicion Am0, Na0;

    std::cout << "--- ANALISIS ROI (TTree Source) ---" << std::endl;
    std::cout << "ROI Am-241: [" << E_Am_Min << " - " << E_Am_Max << "] MeV" << std::endl;
    std::cout << "ROI Na-22:  [" << E_Na_Min << " - " << E_Na_Max << "] MeV" << std::endl;

    for(int i=0; i<n; i++) {
        Medicion Am = IntegrarPico(filesAm[i], E_Am_Min, E_Am_Max);
        Medicion Na = IntegrarPico(filesNa[i], E_Na_Min, E_Na_Max);

        if(i==0) { Am0 = Am; Na0 = Na; }

        if (Am.cuentas <= 0) Am.cuentas = 1e-9;
        if (Na.cuentas <= 0) Na.cuentas = 1e-9;

        double T_Am = Am.cuentas / Am0.cuentas;
        double T_Na = Na.cuentas / Na0.cuentas;
        
        double L_Am = -log(T_Am);
        double L_Na = -log(T_Na);

        double R = (L_Na > 1e-4) ? L_Am / L_Na : 0;
        
        // Error simple (estadística de conteo)
        double term_Am = (Am.cuentas > 0) ? 1.0/Am.cuentas : 0;
        double term_Na = (Na.cuentas > 0) ? 1.0/Na.cuentas : 0;
        double err_R = (R > 0) ? R * sqrt(term_Am + term_Na) : 0; // Aprox muy basica

        if(i==0) { R=0; err_R=0; } // Punto de referencia

        R_val[i] = R;
        R_err[i] = err_R;
        
        std::cout << "C[" << conc[i] << "%] -> R=" << R << std::endl;
    }
    
    TCanvas* c1 = new TCanvas("cROI", "Analisis ROI", 800, 600);
    c1->SetGrid();
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &R_val[0], &x_err[0], &R_err[0]);
    gr->SetTitle("Analisis usando ROIs especificos;Concentracion REE (%);R Value");
    gr->SetMarkerStyle(22);
    gr->SetMarkerColor(kGreen+2);
    gr->Draw("AP");
    c1->SaveAs("Analisis_ROI_TTree.png");
}