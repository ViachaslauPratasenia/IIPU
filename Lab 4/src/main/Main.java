import controller.GlobalScreen;
import controller.WebcamController;

import javax.swing.*;

public class Main {
    private static WebcamController webcamController;

    public static void main(String[] args) {
        webcamController = new WebcamController();
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                new GlobalScreen(webcamController);
            }
        });
    }
}