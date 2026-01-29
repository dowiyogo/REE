#include "TFile.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TStyle.h"
#include <iostream>
#include <vector>
#include <cmath>

// Configuración de ROIs ajustada a MeV (GEANT4 Default)
// Am-241 (59.5 keV) -> 0.0595 MeV
const double E_Am_Min = 0.052; 
const double E_Am_Max = 0.068; 
// Na-22 (511 keV) -> 0.511 MeV
const double E_Na_Min = 0.490;
const double E_Na_Max = 0.532; 

struct Medicion { double cuentas; double error; };

Medicion IntegrarPico(const char* filename, double eMin, double eMax) {
    TFile* f = TFile::Open(filename);
    Medicion m = {0, 0};
    
    if(!f || f->IsZombie()) {
        std::cerr << "Error abriendo: " << filename << std::endl;
        return m;
    }

    TH1D* h = (TH1D*)f->Get("Energy"); 
    if(!h) h = (TH1D*)f->Get("h1");
    
    if(h) {
        // --- AUTO-DETECCIÓN DE UNIDADES ---
        // Si el máximo del eje X es mayor a 100, asumimos keV. Si es menor a 20, asumimos MeV.
        double xMax = h->GetXaxis()->GetXmax();
        double factor = 1.0;
        
        if (xMax > 100.0) { 
            // Estamos en keV, multiplicar ROI por 1000? No, la ROI está en MeV.
            // Convertimos la ROI de entrada (MeV) a keV para buscar en el histo
            factor = 1000.0; 
        }
        
        int binMin = h->GetXaxis()->FindBin(eMin * factor);
        int binMax = h->GetXaxis()->FindBin(eMax * factor);

        double err;
        m.cuentas = h->IntegralAndError(binMin, binMax, err);
        m.error = err;
        
        // Debug para ver si estamos integrando bien
        // std::cout << "Archivo: " << filename << " | Rango: " << eMin*factor << "-" << eMax*factor 
        //           << " | Cuentas: " << m.cuentas << std::endl;
    }
    f->Close();
    return m;
}

void analisis_LaBr3_ROI() {
    gStyle->SetOptFit(1);
    
    std::vector<double> conc = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    const int n = conc.size();

    // Archivos
    std::vector<const char*> filesAm = { "Am241_0_REE.root", "Am241_1_REE.root", "Am241_2_REE.root", "Am241_3_REE.root", "Am241_4_REE.root", "Am241_5_REE.root" };
    std::vector<const char*> filesNa = { "Na22_0_REE.root", "Na22_1_REE.root", "Na22_2_REE.root", "Na22_3_REE.root", "Na22_4_REE.root", "Na22_5_REE.root" };

    std::vector<double> R_val(n), R_err(n), x_err(n, 0.0);
    Medicion Am0, Na0;

    std::cout << "--- ANALISIS ROI (Unidades Corregidas) ---" << std::endl;

    for(int i=0; i<n; i++) {
        Medicion Am = IntegrarPico(filesAm[i], E_Am_Min, E_Am_Max);
        Medicion Na = IntegrarPico(filesNa[i], E_Na_Min, E_Na_Max);

        if(i==0) { Am0 = Am; Na0 = Na; }

        // Protección contra ceros (si la simulación falló)
        if (Am.cuentas <= 0) Am.cuentas = 1e-9;
        if (Na.cuentas <= 0) Na.cuentas = 1e-9;

        double T_Am = Am.cuentas / Am0.cuentas;
        double T_Na = Na.cuentas / Na0.cuentas;
        
        double L_Am = -log(T_Am);
        double L_Na = -log(T_Na);

        double R = (L_Na > 1e-4) ? L_Am / L_Na : 0;
        
        // Error simple
        double term_Am = 1.0/Am.cuentas;
        double err_R = (R > 0) ? R * sqrt(term_Am) : 0;

        if(i==0) { R=0; err_R=0; } // Referencia visual

        R_val[i] = R;
        R_err[i] = err_R;

        std::cout << "Conc: " << conc[i] << "% | Cuentas Am: " << Am.cuentas << " | R: " << R << std::endl;
    }

    TCanvas* c1 = new TCanvas("cROIv2", "Analisis LaBr3 ROI v2", 800, 600);
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &R_val[0], &x_err[0], &R_err[0]);
    
    gr->SetTitle("Sensibilidad LaBr3 (ROI Correcta);Concentracion REE (%);Ratio R");
    gr->SetMarkerStyle(21);
    gr->SetMarkerColor(kBlue);
    gr->Draw("AP");

    TF1* f = new TF1("f", "pol1", 0.9, 3.1);
    gr->Fit("f", "R");
}