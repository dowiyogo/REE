// debug_transmision.cpp
void debug_transmision() {
    // Tus archivos
    std::vector<const char*> filesAm = { "Am241_0_REE.root", "Am241_1_REE.root", "Am241_2_REE.root", "Am241_3_REE.root", "Am241_4_REE.root", "Am241_5_REE.root" };
    std::vector<const char*> filesNa = { "Na22_0_REE.root", "Na22_1_REE.root", "Na22_2_REE.root", "Na22_3_REE.root", "Na22_4_REE.root", "Na22_5_REE.root" };
    
    double I0_Am = 0, I0_Na = 0;

    std::cout << "Conc(%) \t I_Am \t\t T_Am \t\t I_Na \t\t T_Na \t\t R_calc" << std::endl;
    std::cout << "------------------------------------------------------------------------" << std::endl;

    for(int i=0; i<6; i++) {
        // Cargar Am
        TFile* fA = TFile::Open(filesAm[i]);
        TH1D* hA = (TH1D*)fA->Get("Energy"); // O "h1"
        double countsA = hA->Integral();
        fA->Close();

        // Cargar Na
        TFile* fN = TFile::Open(filesNa[i]);
        TH1D* hN = (TH1D*)fN->Get("Energy"); // O "h1"
        double countsN = hN->Integral();
        fN->Close();

        if(i==0) { I0_Am = countsA; I0_Na = countsN; }

        double T_Am = countsA/I0_Am;
        double T_Na = countsN/I0_Na;
        double R = (i==0) ? 0 : log(T_Am)/log(T_Na); // Evitar div por 0

        printf("%d%% \t\t %.0f \t %.4f \t %.0f \t %.4f \t %.2f\n", i, countsA, T_Am, countsN, T_Na, R);
    }
}