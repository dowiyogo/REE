/**
 * @file DiagnosticoEu152.cpp
 * @brief Diagnóstico de espectros Eu-152 para identificar picos disponibles
 */

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TSpectrum.h"
#include "TStyle.h"
#include <iostream>
#include <algorithm>
#include <vector>

void DiagnosticoEu152(const char* archivo = "Eu152_REE_0p00.root") {
    
    gStyle->SetOptStat(111111);
    
    TFile* f = TFile::Open(archivo);
    if (!f || f->IsZombie()) {
        std::cerr << "No se puede abrir: " << archivo << std::endl;
        return;
    }
    
    TTree* t = (TTree*)f->Get("Scoring");
    if (!t) {
        std::cerr << "No se encontró TTree Scoring" << std::endl;
        return;
    }
    
    std::cout << "\n=== DIAGNÓSTICO DEL ESPECTRO ===" << std::endl;
    std::cout << "Archivo: " << archivo << std::endl;
    std::cout << "Entradas en TTree: " << t->GetEntries() << std::endl;
    
    // Ver las ramas disponibles
    std::cout << "\nRamas disponibles:" << std::endl;
    t->Print();
    
    // Histograma amplio para ver todo el espectro
    TCanvas* c1 = new TCanvas("c1", "Diagnostico", 1400, 500);
    c1->Divide(2, 1);
    
    // Panel 1: Espectro completo
    c1->cd(1);
    gPad->SetLogy();
    
    TH1D* hFull = new TH1D("hFull", "Espectro Completo;Energia (keV);Cuentas", 
                           2000, 0, 2000);
    t->Draw("Energy*1000>>hFull", "", "goff");
    hFull->SetLineColor(kBlue);
    hFull->Draw();
    
    std::cout << "\nEstadísticas del histograma:" << std::endl;
    std::cout << "  Entradas: " << hFull->GetEntries() << std::endl;
    std::cout << "  Media: " << hFull->GetMean() << " keV" << std::endl;
    std::cout << "  RMS: " << hFull->GetRMS() << " keV" << std::endl;
    std::cout << "  Máximo en: " << hFull->GetBinCenter(hFull->GetMaximumBin()) << " keV" << std::endl;
    
    // Buscar picos con TSpectrum (parámetros más conservadores)
    TSpectrum* spec = new TSpectrum(30);
    Int_t nfound = spec->Search(hFull, 3, "", 0.05);  // sigma=3, threshold=5%
    
    std::cout << "\n=== PICOS ENCONTRADOS (" << nfound << ") ===" << std::endl;
    
    Double_t* xpeaks = spec->GetPositionX();
    Double_t* ypeaks = spec->GetPositionY();
    
    // Ordenar picos por energía
    std::vector<std::pair<double, double>> picos;
    for (Int_t i = 0; i < nfound; i++) {
        picos.push_back({xpeaks[i], ypeaks[i]});
    }
    std::sort(picos.begin(), picos.end());
    
    // Líneas teóricas del Eu-152
    double Eu152_teorico[] = {121.78, 244.70, 344.28, 411.12, 443.96, 
                              778.90, 867.38, 964.08, 1085.87, 1112.07, 1408.01};
    int n_teorico = 11;
    
    std::cout << "\nEnergia (keV)  |  Cuentas  |  Linea Eu-152 mas cercana" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (auto& p : picos) {
        // Buscar línea teórica más cercana
        double min_diff = 1e9;
        double linea_cercana = 0;
        for (int j = 0; j < n_teorico; j++) {
            double diff = std::abs(p.first - Eu152_teorico[j]);
            if (diff < min_diff) {
                min_diff = diff;
                linea_cercana = Eu152_teorico[j];
            }
        }
        
        printf("%12.2f  |  %8.0f  |  %.2f keV (diff: %.1f)\n", 
               p.first, p.second, linea_cercana, min_diff);
    }
    
    // Panel 2: Zoom en regiones de interés
    c1->cd(2);
    
    // Verificar contenido en regiones específicas
    std::cout << "\n=== CONTENIDO EN REGIONES DE INTERÉS ===" << std::endl;
    
    // Región 122 keV
    int bin1 = hFull->FindBin(100);
    int bin2 = hFull->FindBin(140);
    double counts_122 = hFull->Integral(bin1, bin2);
    std::cout << "  100-140 keV (pico 122): " << counts_122 << " cuentas" << std::endl;
    
    // Región 344 keV
    bin1 = hFull->FindBin(320);
    bin2 = hFull->FindBin(370);
    double counts_344 = hFull->Integral(bin1, bin2);
    std::cout << "  320-370 keV (pico 344): " << counts_344 << " cuentas" << std::endl;
    
    // Región 779 keV
    bin1 = hFull->FindBin(750);
    bin2 = hFull->FindBin(810);
    double counts_779 = hFull->Integral(bin1, bin2);
    std::cout << "  750-810 keV (pico 779): " << counts_779 << " cuentas" << std::endl;
    
    // Región 1408 keV
    bin1 = hFull->FindBin(1380);
    bin2 = hFull->FindBin(1440);
    double counts_1408 = hFull->Integral(bin1, bin2);
    std::cout << "  1380-1440 keV (pico 1408): " << counts_1408 << " cuentas" << std::endl;
    
    // Dibujar zoom en región 1408
    TH1D* hZoom = new TH1D("hZoom", "Zoom 1300-1500 keV;Energia (keV);Cuentas", 
                           200, 1300, 1500);
    t->Draw("Energy*1000>>hZoom", "Energy*1000>1300 && Energy*1000<1500", "goff");
    hZoom->SetLineColor(kRed);
    hZoom->Draw();
    
    c1->SaveAs("Diagnostico_Eu152.png");
    
    std::cout << "\n=== RECOMENDACIÓN ===" << std::endl;
    
    if (counts_1408 < 100) {
        std::cout << "⚠ POCA ESTADÍSTICA en 1408 keV (" << counts_1408 << " cuentas)" << std::endl;
        std::cout << "  Considera usar otra línea de alta energía:" << std::endl;
        std::cout << "  - 779 keV (" << counts_779 << " cuentas)" << std::endl;
        std::cout << "  - 344 keV (" << counts_344 << " cuentas) como alternativa" << std::endl;
    }
    
    std::cout << "\nGráfico guardado: Diagnostico_Eu152.png" << std::endl;
}
