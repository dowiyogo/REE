#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include "TStyle.h"
#include "TMath.h"
#include "TPaveText.h"
#include <iostream>
#include <vector>
#include <cmath>

// --- FUNCIÓN AUXILIAR PARA OBTENER CUENTAS E INTEGRAR DESDE TREE ---
double ObtenerCuentas(const char* nombreArchivo, double &error) {
    TFile* f = TFile::Open(nombreArchivo);
    if(!f || f->IsZombie()) {
        std::cerr << "[ERROR] No se pudo abrir: " << nombreArchivo << std::endl;
        return 0;
    }
    
    // Accedemos al TTree
    TTree* t = (TTree*)f->Get("Scoring");
    if(!t) {
        std::cerr << "[ERROR] TTree 'Scoring' no encontrado en " << nombreArchivo << std::endl;
        f->Close();
        return 0;
    }

    // Creamos histograma temporal para proyección
    // Usamos un binning fino para asegurar precisión
    TH1D* h = new TH1D("hTemp", "Spectrum", 2000, 0, 2.0); // 0 a 2 MeV
    t->Draw("Energy>>hTemp", "", "goff");

    // Integramos todo el espectro (o definiríamos ROI aquí)
    double cuentas = h->IntegralAndError(1, h->GetNbinsX(), error);
    
    delete h;
    f->Close();
    return cuentas;
}

void analisis_Am241() {
    // Estilo
    gStyle->SetOptFit(1111); 
    
    const int n = 6;
    double conc[n] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}; // % de REE
    
    // Archivos
    std::vector<const char*> archivos = {
        "Am241_0p0_REE.root", "Am241_0p01_REE.root", "Am241_0p02_REE.root", 
        "Am241_0p03_REE.root", "Am241_0p04_REE.root", "Am241_0p05_REE.root"
    };

    std::vector<double> I_Am(n), e_Am(n);
    std::vector<double> T_Am(n), eT_Am(n);

    // 1. Obtener Cuentas I0 (Referencia 0%)
    double I0, e0;
    I0 = ObtenerCuentas(archivos[0], e0);
    
    if (I0 <= 0) {
        std::cerr << "Error: I0 es cero o negativo. Verifica simulación." << std::endl;
        return;
    }

    std::cout << "Referencia (0% REE): " << I0 << " +/- " << e0 << " cps (simulado)" << std::endl;

    // 2. Loop sobre concentraciones
    for(int i=0; i<n; i++) {
        I_Am[i] = ObtenerCuentas(archivos[i], e_Am[i]);
        
        // Cálculo de Transmisión T = I / I0
        T_Am[i] = I_Am[i] / I0;

        // Propagación de Error:
        // sigma_T = T * sqrt( (sigma_I/I)^2 + (sigma_I0/I0)^2 )
        double term1 = (I_Am[i] > 0) ? pow(e_Am[i]/I_Am[i], 2) : 0;
        double term2 = pow(e0/I0, 2);
        eT_Am[i] = T_Am[i] * sqrt(term1 + term2);

        std::cout << "Conc " << conc[i] << "% -> T = " << T_Am[i] << " +/- " << eT_Am[i] << std::endl;
    }

    // 3. Gráfico
    TCanvas* c1 = new TCanvas("c1", "Analisis Am-241", 900, 600);
    c1->SetGrid();

    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &T_Am[0], 0, &eT_Am[0]);
    gr->SetTitle("Atenuacion Gamma vs Concentracion REE;Concentracion REE (%);Transmision Relativa (I/I_{0})");
    gr->SetMarkerStyle(21); // Cuadrados llenos
    gr->SetMarkerSize(1.2);
    gr->SetMarkerColor(kBlue+1);
    gr->SetLineColor(kBlue+1);
    gr->SetLineWidth(2);

    // Ajuste Exponencial: Ley de Beer-Lambert Modificada
    // T = p0 * exp(-p1 * x)
    TF1* fFit = new TF1("fFit", "[0]*exp(-[1]*x)", 0, 5.5);
    fFit->SetParameters(1.0, 0.05); 
    fFit->SetParName(0, "Norm (T0)");
    fFit->SetParName(1, "Sensibilidad (k)");
    fFit->SetLineColor(kRed);

    gr->Fit("fFit", "R"); 

    gr->Draw("AP"); 

    // 5. Mostrar Ecuación
    double p0 = fFit->GetParameter(0);
    double p1 = fFit->GetParameter(1);
    
    TPaveText *pt = new TPaveText(0.5, 0.6, 0.85, 0.8, "NDC");
    pt->SetFillColor(kWhite);
    pt->AddText(Form("Ajuste: T = %.2f e^{-%.3f x}", p0, p1));
    pt->Draw();

    c1->SaveAs("Analisis_Am241_Fit.png");
}