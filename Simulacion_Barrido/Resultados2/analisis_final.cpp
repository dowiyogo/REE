#include "TFile.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TPaveText.h" // Agregado para mostrar ecuaciones
#include "TMath.h"
#include <iostream>
#include <vector>
#include <cmath>

// Estructura simple
struct Medicion { double cuentas; double error; };

Medicion ObtenerCuentas(const char* nombreArchivo) {
    TFile* f = TFile::Open(nombreArchivo);
    Medicion m = {0, 0};
    if(!f || f->IsZombie()) return m;
    
    // Intenta nombres comunes de histogramas
    TH1D* h = (TH1D*)f->Get("Energy"); 
    if(!h) h = (TH1D*)f->Get("h1");
    
    if(h) {
        double err;
        m.cuentas = h->IntegralAndError(1, h->GetNbinsX(), err);
        m.error = err;
    }
    f->Close();
    return m;
}

void analisis_final() {
    // Estética Profesional
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0); // Lo haremos manual para que se vea bonito
    gStyle->SetLabelSize(0.04, "XY");
    gStyle->SetTitleSize(0.05, "XY");

    // Datos
    std::vector<double> conc = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    const int n = conc.size();
    
    std::vector<const char*> filesAm = { "Am241_0_REE.root", "Am241_1_REE.root", "Am241_2_REE.root", "Am241_3_REE.root", "Am241_4_REE.root", "Am241_5_REE.root" };
    std::vector<const char*> filesNa = { "Na22_0_REE.root", "Na22_1_REE.root", "Na22_2_REE.root", "Na22_3_REE.root", "Na22_4_REE.root", "Na22_5_REE.root" };

    std::vector<double> R_val(n), R_err(n), x_err(n, 0.0);
    Medicion Am0, Na0;

    // --- PROCESAMIENTO ---
    for(int i=0; i<n; i++) {
        Medicion Am = ObtenerCuentas(filesAm[i]);
        Medicion Na = ObtenerCuentas(filesNa[i]);

        if(i==0) { Am0 = Am; Na0 = Na; }

        double T_Am = Am.cuentas / Am0.cuentas;
        double T_Na = Na.cuentas / Na0.cuentas;

        // Protección contra ceros
        if(T_Am < 1e-5) T_Am = 1e-5; 

        double L_Am = -log(T_Am);
        double L_Na = -log(T_Na);

        // Ratio
        double R = 0;
        double err_R = 0;

        if (i == 0) {
            // Singularidad matemática en 0/0. Asignamos 0 o extrapolamos visualmente.
            // Para el gráfico, lo pondremos en 0, pero NO lo ajustaremos.
            R = 0.0; 
            err_R = 0.0; 
        } else {
            R = L_Am / L_Na;
            // Propagación Error Simplificada (Dominada por Am)
            // dR/R approx dL_Am/L_Am approx dT_Am/(T_Am * ln T_Am)
            double term_Am = 1.0/Am.cuentas; // Error relativo cuadrado de cuentas (Poisson)
            double err_L_Am = sqrt(term_Am); // Error absoluto en Log
            err_R = R * (err_L_Am / L_Am);   // Error absoluto en R
        }

        R_val[i] = R;
        R_err[i] = err_R;
    }

    // --- GRAFICACIÓN ---
    TCanvas* c1 = new TCanvas("cFinal", "Sistema Dual-Energy Final", 900, 700);
    c1->SetGrid();
    c1->SetLeftMargin(0.12);

    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &R_val[0], &x_err[0], &R_err[0]);
    gr->SetTitle("Calibracion Final Tierras Raras;Concentracion REE (%);Ratio R (Sensibilidad al Z)");
    
    // Estilo de puntos
    gr->SetMarkerStyle(21); // Cuadrado
    gr->SetMarkerSize(1.5);
    gr->SetMarkerColor(kBlue+2);
    gr->SetLineColor(kBlue+2);
    gr->SetLineWidth(2);
    gr->Draw("AP");

    // --- EL AJUSTE MAESTRO (SOLO RANGO LINEAL) ---
    // Ajustamos solo de 0.8% a 3.2% (Ignoramos el 0% singular y la saturación > 3.5%)
    TF1* fit = new TF1("fLinear", "[0] + [1]*x", 0.8, 3.2);
    fit->SetLineColor(kRed);
    fit->SetLineWidth(3);
    fit->SetLineStyle(1);
    
    gr->Fit("fLinear", "R");

    // --- VISUALIZACIÓN DE ZONAS ---
    // Extrapolamos la línea roja para ver cómo se desvía la realidad
    TF1* fit_extrap = (TF1*)fit->Clone("fExtrap");
    fit_extrap->SetRange(0, 5.5);
    fit_extrap->SetLineStyle(2); // Punteada
    fit_extrap->SetLineWidth(2);
    fit_extrap->Draw("SAME");

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
    pt->AddText(Form("#chi^{2}/ndf = %.2f", chi2/ndf));
    pt->AddText("Rango Lineal: 1% - 3%");
    pt->Draw();

    // Leyenda
    TLegend *leg = new TLegend(0.15, 0.7, 0.45, 0.85);
    leg->AddEntry(gr, "Datos Simulados (Geant4)", "ep");
    leg->AddEntry(fit, "Ajuste Lineal (Zona Valida)", "l");
    leg->AddEntry(fit_extrap, "Proyeccion Teorica", "l");
    leg->Draw();

    c1->SaveAs("Curva_Calibracion_Final.png");

    // --- REPORTE DE CALIDAD ---
    std::cout << "\n=== RESULTADOS FINALES ===" << std::endl;
    std::cout << "Sensibilidad: " << p1 << " unidades de R por % de REE" << std::endl;
    std::cout << "Offset (p0): " << p0 << std::endl;
    
    // Calculo del error de estimacion al 2% (centro del rango)
    double error_R_2percent = R_err[2]; 
    double error_Conc = error_R_2percent / p1;
    
    std::cout << "Error en R al 2%: +/- " << error_R_2percent << std::endl;
    std::cout << "Precision estimada en concentracion: +/- " << error_Conc << "%" << std::endl;
}