#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TPaveText.h" 
#include "TMath.h"
#include <iostream>
#include <vector>
#include <cmath>

struct Medicion { double cuentas; double error; };

Medicion ObtenerCuentas(const char* nombreArchivo) {
    TFile* f = TFile::Open(nombreArchivo);
    Medicion m = {0, 0};
    if(!f || f->IsZombie()) return m;
    
    // Adaptación para TTree Scoring
    TTree* t = (TTree*)f->Get("Scoring");
    if(t) {
        TH1D* h = new TH1D("hTemp", "Energy", 4096, 0, 3.0);
        t->Draw("Energy>>hTemp", "", "goff");
        
        double err;
        m.cuentas = h->IntegralAndError(1, h->GetNbinsX(), err);
        m.error = err;
        delete h;
    }
    f->Close();
    return m;
}

void analisis_final() {
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);
    gStyle->SetLabelSize(0.04, "XY");
    gStyle->SetTitleSize(0.05, "XY");

    // Datos simulados (placeholder para estructura de archivos real)
    const int n = 6;
    double conc[n] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    
    std::vector<const char*> filesAm = {
        "Am241_0p0_REE.root", "Am241_0p01_REE.root", "Am241_0p02_REE.root", 
        "Am241_0p03_REE.root", "Am241_0p04_REE.root", "Am241_0p05_REE.root"
    };
    std::vector<const char*> filesNa = {
        "Na22_0p0_REE.root", "Na22_0p01_REE.root", "Na22_0p02_REE.root", 
        "Na22_0p03_REE.root", "Na22_0p04_REE.root", "Na22_0p05_REE.root" 
    };

    std::vector<double> R_val(n), R_err(n), x_err(n, 0.0);
    Medicion Am0 = ObtenerCuentas(filesAm[0]);
    Medicion Na0 = ObtenerCuentas(filesNa[0]);

    for(int i=0; i<n; i++) {
        Medicion Am = ObtenerCuentas(filesAm[i]);
        Medicion Na = ObtenerCuentas(filesNa[i]);

        double T_Am = (Am0.cuentas>0) ? Am.cuentas/Am0.cuentas : 0;
        double T_Na = (Na0.cuentas>0) ? Na.cuentas/Na0.cuentas : 0;
        
        if(T_Am <= 0) T_Am = 1e-9; 
        if(T_Na <= 0) T_Na = 1e-9;

        double L_Am = -log(T_Am);
        double L_Na = -log(T_Na);

        double R = (L_Na > 0.001) ? L_Am / L_Na : 0; 

        // Error simplificado
        double err_R = (R>0) ? R * 0.05 : 0; // Asumiendo 5% error relativo para demo visual

        R_val[i] = R;
        R_err[i] = err_R;
    }

    TCanvas* c1 = new TCanvas("cFinal", "Reporte Final", 1000, 700);
    c1->SetGrid();
    
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &R_val[0], &x_err[0], &R_err[0]);
    gr->SetTitle("Curva de Calibracion Final;Concentracion REE (%);Indice R");
    gr->SetMarkerStyle(21);
    gr->SetMarkerSize(1.5);
    gr->SetMarkerColor(kAzure+2);
    gr->SetLineColor(kAzure+2);
    gr->Draw("AP");

    TF1* fit = new TF1("fLin", "pol1", 0.5, 5.5);
    fit->SetLineColor(kOrange+7);
    fit->SetLineWidth(3);
    gr->Fit("fLin", "R");

    // --- CAJA DE RESULTADOS ---
    double p0 = fit->GetParameter(0);
    double p1 = fit->GetParameter(1);
    double chi2 = fit->GetChisquare();
    double ndf = fit->GetNDF();

    TPaveText *pt = new TPaveText(0.5, 0.15, 0.88, 0.45, "NDC");
    pt->SetFillColor(kWhite);
    pt->SetBorderSize(1);
    pt->SetTextAlign(12);
    pt->AddText("Modelo de Calibracion:");
    pt->AddText(Form("R = %.2f + %.2f #cdot C_{REE}", p0, p1));
    pt->AddText("----------------");
    pt->AddText(Form("#chi^{2}/ndf = %.2f", (ndf>0 ? chi2/ndf : 0)));
    pt->Draw();

    TLegend *leg = new TLegend(0.15, 0.7, 0.45, 0.85);
    leg->AddEntry(gr, "Datos Simulados (Geant4)", "ep");
    leg->AddEntry(fit, "Ajuste Lineal", "l");
    leg->Draw();

    c1->SaveAs("Curva_Calibracion_Final.png");
}