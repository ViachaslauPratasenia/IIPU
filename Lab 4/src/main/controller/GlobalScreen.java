package controller;

import java.awt.*;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.*;

import org.jnativehook.NativeHookException;
import org.jnativehook.dispatcher.SwingDispatchService;
import org.jnativehook.keyboard.NativeKeyEvent;
import org.jnativehook.keyboard.NativeKeyListener;

import static org.jnativehook.GlobalScreen.*;


public class GlobalScreen extends JFrame implements NativeKeyListener, WindowListener {
    private static final int FPS = 60;
    private static WebcamController webcamController;
    public GlobalScreen(WebcamController controller) {
        webcamController = controller;
        // Set the event dispatcher to a swing safe executor service.
        setEventDispatcher(new SwingDispatchService());
        setSize(300, 150);
        setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
        addWindowListener(this);
        setVisible(true);
        hide();

        // Get the logger for "org.jnativehook" and set the level to off.
        Logger logger = Logger.getLogger(org.jnativehook.GlobalScreen.class.getPackage().getName());
        logger.setLevel(Level.WARNING);

        logger.setUseParentHandlers(false);
        //hide();
    }

    public void windowOpened(WindowEvent e) {
        // Initialize native hook.
        try {
            registerNativeHook();
        }
        catch (NativeHookException ex) {
            System.err.println("There was a problem registering the native hook.");
            System.err.println(ex.getMessage());
            ex.printStackTrace();

            System.exit(1);
        }

        addNativeKeyListener(this);
    }

    public void windowClosed(WindowEvent e) {
        //Clean up the native hook.
        try {
            unregisterNativeHook();
        } catch (NativeHookException e1) {
            e1.printStackTrace();
        }
        System.runFinalization();
        System.exit(0);
    }

    public void nativeKeyReleased(NativeKeyEvent e) {
        if (e.getKeyCode() == NativeKeyEvent.VC_Q ) {
            JOptionPane.showMessageDialog(null, "Webcam: " + webcamController.getName());
        }
        if (e.getKeyCode() == NativeKeyEvent.VC_W) {
            try {
                webcamController.takePhoto();
            } catch (IOException e1) {
                e1.printStackTrace();
            }
            JOptionPane.showMessageDialog(null, "Photo had been taken");
        }
        if (e.getKeyCode() == NativeKeyEvent.VC_E ) {
            int duration = showDurationInputDialog();
            try {
                webcamController.recordVideo(duration, FPS);
                JOptionPane.showMessageDialog(null, "Video was recorded");
            } catch (Exception e1) {
                JOptionPane.showMessageDialog(null, "Null pointer exception");
            }
        }
        if(e.getKeyCode() == NativeKeyEvent.VC_R) {
            JOptionPane.showMessageDialog(null, "Webcam is closed");
            try {
                unregisterNativeHook();
            } catch (NativeHookException e1) {
                e1.printStackTrace();
            }
            System.runFinalization();
            System.exit(0);
        }
    }

    public void windowClosing(WindowEvent e) { /* Unimplemented */ }
    public void windowIconified(WindowEvent e) { /* Unimplemented */ }
    public void windowDeiconified(WindowEvent e) { /* Unimplemented */ }
    public void windowActivated(WindowEvent e) { /* Unimplemented */ }
    public void windowDeactivated(WindowEvent e) { /* Unimplemented */ }
    public void nativeKeyPressed(NativeKeyEvent e) { /* Unimplemented */ }
    public void nativeKeyTyped(NativeKeyEvent e) { /* Unimplemented */ }

    private static int showDurationInputDialog() {
        int result;
        String inputValue = JOptionPane.showInputDialog("Please input required video duration", JOptionPane.OK_OPTION);
        try {
            result = Integer.parseUnsignedInt(inputValue);
        } catch (Exception exception){
            JOptionPane.showMessageDialog(null, "input value is invalid");
            result = showDurationInputDialog();
        }
        return result;
    }
}
