import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.security.*;
import java.util.*;
import javax.swing.*;

class P {
    public int r, c;
    public P() {
        this.r = this.c = -1;
    }
    public P(int r, int c) {
        this.r = r;
        this.c = c;
    }
    public P(P p) {
        this.r = p.r;
        this.c = p.c;
    }
}

public class RollingBallsVis {
    static int maxSize = 60, minSize = 10;
    static int maxWallsP = 30, minWallsP = 10;
    static int maxBallsP = 20, minBallsP = 5;
    static int maxRolls;

    final int[] dr = {0, 1, 0, -1};
    final int[] dc = {-1, 0, 1, 0};

    int W, H, nBalls;
    char[][] targetMaze;
    char[][] startMaze;

    int prevX1, prevY1;
    int prevX2, prevY2;
    static boolean startPaused = false;

    SecureRandom r1;

    // used for simulation / manual play
    volatile char[][] curMaze;
    // -----------------------------------------
    boolean isInside(P p) {
        return (p.r >= 0 && p.r < H && p.c >= 0 && p.c < W);
    }
    // -----------------------------------------
    P roll(P p, int dir) {
        // perform a roll of ball at P in direction dir
        // do the change in curMaze
        prevX1 = p.c;
        prevY1 = p.r;
        P newP = new P(p);
        do {
            newP.r += dr[dir];
            newP.c += dc[dir];
        } while (isInside(newP) && curMaze[newP.r][newP.c] == '.');
        newP.r -= dr[dir];
        newP.c -= dc[dir];
        if (newP.r != p.r || newP.c != p.c) {
            curMaze[newP.r][newP.c] = curMaze[p.r][p.c];
            curMaze[p.r][p.c] = '.';
        }
        prevX2 = newP.c;
        prevY2 = newP.r;
        return newP;
    }
    // -----------------------------------------
    int chooseRoll(P ballP) {
        // find all allowed roll directions for this ball, and return a random one or -1 if none exist
        int[] allowedDir = new int[4];
        int nAllow = 0;
        for (int dir = 0; dir < 4; ++dir) {
            P adjP = new P(ballP);
            adjP.r += dr[dir];
            adjP.c += dc[dir];
            if (isInside(adjP) && curMaze[adjP.r][adjP.c] == '.') {
                allowedDir[nAllow] = dir;
                ++nAllow;
            }
        }
        if (nAllow == 0)
            return -1;
        return allowedDir[r1.nextInt(nAllow)];
    }
    // -----------------------------------------
    void generate(long seed) {
      try {
        // generate test case
        r1 = SecureRandom.getInstance("SHA1PRNG"); 
        r1.setSeed(seed);
        H = r1.nextInt(maxSize - minSize + 1) + minSize;
        W = r1.nextInt(maxSize - minSize + 1) + minSize;
        int C = r1.nextInt(10) + 1;
        if (seed <= 3) {
            W = H = minSize * (int)seed;
            C = (int)seed;
        }
        System.out.println("H = " + H);
        System.out.println("W = " + W);
        System.out.println("C = " + C);

        // generate the original maze (= target)
        int wallsPercentage = r1.nextInt(maxWallsP - minWallsP + 1) + minWallsP;
        int ballsPercentage = r1.nextInt(maxBallsP - minBallsP + 1) + minBallsP;
        nBalls = 0;
        do {
            targetMaze = new char[H][W];
            for (int i = 0; i < H; i++)
            for (int j = 0; j < W; j++) {
                if (r1.nextInt(100) < wallsPercentage) {
                    targetMaze[i][j] = '#';
                } else {
                    // an empty space or a ball
                    if (r1.nextInt(100) < ballsPercentage) {
                        targetMaze[i][j] = (char)((int)'0' + r1.nextInt(C));
                        ++nBalls;
                    } else {
                        targetMaze[i][j] = '.';
                    }
                }
            }
            // make sure there's at least 1 ball in the maze
        } while (nBalls == 0);

        curMaze = new char[H][W];
        for (int i = 0; i < H; ++i)
            curMaze[i] = Arrays.copyOf(targetMaze[i], W);

        // store ball coords more conveniently
        P[] ball = new P[nBalls];
        int ballInd = 0;
        for (int i = 0; i < H; i++)
        for (int j = 0; j < W; j++)
            if (curMaze[i][j] != '#' && curMaze[i][j] != '.') {
                ball[ballInd] = new P(i, j);
                ++ballInd;
            }
        // do random rolls
        int nRolls = r1.nextInt(nBalls * 10) + nBalls * 3;
        for (int roll = 0; roll < nRolls; ++roll) {
            int i = r1.nextInt(nBalls);
            int choice = chooseRoll(ball[i]);
            if (choice > -1) {
                ball[i] = roll(ball[i], choice);
            }
        }
        // try to get balls out of their target positions (best effort)
        for (int i = 0; i < nBalls; ++i) {
            int t = targetMaze[ball[i].r][ball[i].c];
            if (t == curMaze[ball[i].r][ball[i].c] && t != '.' && t != '#') {
                int choice = chooseRoll(ball[i]);
                if (choice > -1) {
                    ball[i] = roll(ball[i], choice);
                }
            }
        }

        startMaze = new char[H][W];
        for (int i = 0; i < H; ++i)
            startMaze[i] = Arrays.copyOf(curMaze[i], W);

        System.out.println("Number of balls = " + nBalls);
        maxRolls = nBalls * 20;
        System.out.println("Max rolls allowed = " + maxRolls);
      }
      catch (Exception e) {
        System.err.println("An exception occurred while generating test case.");
        e.printStackTrace(); 
      }
    }
    // -----------------------------------------
    double getScore() {
        double score = 0;
        // for each ball in target configuration, if there is a ball of wrong color there, give 0.5 point
        // if the ball is of correct color, give 1 point
        // divide by total number of balls (max score)
        for (int i = 0; i < H; ++i)
        for (int j = 0; j < W; ++j)
            if (targetMaze[i][j] != '#' && targetMaze[i][j] != '.')
                if (curMaze[i][j] == targetMaze[i][j]) {
                    score += 1;
                } else if (curMaze[i][j] != '#' && curMaze[i][j] != '.') {
                    score += 0.5;
                }
        return score / nBalls;
    }
    // -----------------------------------------
    public double runTest(String seed) {
      try {
        generate(Long.parseLong(seed));

        if (vis) {
            if (startPaused) {
                v.pauseMode = true;
            }
            prevX1 = -1;
            jf.setSize((W+3)*SZ+100,H*SZ+40);
            jf.setVisible(true);
            manualReady = false;
            draw();
        }

        // call the solution
        String[] startMazeStr, targetMazeStr;
        startMazeStr = new String[H];
        if (debug)
            System.out.println("Start maze:");
        for (int i = 0; i < H; ++i) {
            startMazeStr[i] = new String(startMaze[i]);
            if (debug)
                System.out.println(startMazeStr[i]);
        }
        targetMazeStr = new String[H];
        if (debug)
            System.out.println("Target maze:");
        for (int i = 0; i < H; ++i) {
            targetMazeStr[i] = new String(targetMaze[i]);
            if (debug)
                System.out.println(targetMazeStr[i]);
        }

        String[] rolls = restorePattern(startMazeStr, targetMazeStr);

        // check the return and score it
        if (rolls == null) {
            addFatalError("Your return contained invalid number of elements.");
            return 0.0;
        }
        if (rolls.length > maxRolls) {
            addFatalError("Your return contained more than " + maxRolls + " elements.");
            return 0.0;
        }

        // parse and simulate rolls starting with startMaze
        // manual mode just returns 0 rolls to process
        for (int i = 0; i < rolls.length; ++i) {
            String[] s = rolls[i].split(" ");
            int R, C, D;
            if (s.length != 3) {
                addFatalError("Element " + i + " of your return must be formatted as \"R C D\"");
                return 0;
            }
            // check the cell we want to start roll from
            try {
                R = Integer.parseInt(s[0]);
                C = Integer.parseInt(s[1]);
                D = Integer.parseInt(s[2]);
            }
            catch (Exception e) {
                addFatalError("R, C and D in element " + i + " of your return must be integers.");
                return 0;
            }
            if (!isInside(new P(R, C))) {
                addFatalError("R and C in element " + i + " of your return must specify a cell within the maze.");
                return 0;
            }
            if (curMaze[R][C] == '.' || curMaze[R][C] == '#') {
                addFatalError("Each roll of your return must start in a cell with a ball.");
                return 0;
            }
            if (D < 0 || D > 3) {
                addFatalError("D must be an integer between 0 and 3, inclusive.");
                return 0;
            }
            // unlike manual play, we don't check that the roll is non-trivial, rolling into a wall is fine

            // finally, perform the roll
            roll(new P(R, C), D);
            manualRolls++;

            if (vis) {
                draw();
            }
        }

        return getScore();
      }
      catch (Exception e) { 
        System.err.println("An exception occurred while trying to get your program's results.");
        e.printStackTrace(); 
        return 0;
      }
    }
// ------------- visualization part ------------
    JFrame jf;
    Vis v;
    static String exec;
    static boolean vis, manual, debug;
    static Process proc;
    static int del;
    InputStream is;
    OutputStream os;
    BufferedReader br;
    static int SZ;
    volatile boolean manualReady;
    volatile boolean firstClick = true;
    volatile P first = new P();
    volatile int manualRolls = 0;
    final static int[] colors = {0x4000FF, 0xFF00BF, 0x00BFFF, 0xFFD761, 0xFF4000, 0x40FF00, 0xB3002D, 0x009973, 0x999999, 0x404040};
    // -----------------------------------------
    String[] restorePattern(String[] start, String[] target) throws IOException {
        if (!manual && proc != null) {
            StringBuffer sb = new StringBuffer();
            sb.append(H).append("\n");
            for (int i = 0; i < H; ++i) {
                sb.append(start[i]).append("\n");
            }
            sb.append(H).append("\n");
            for (int i = 0; i < H; ++i) {
                sb.append(target[i]).append("\n");
            }
            os.write(sb.toString().getBytes());
            os.flush();
        }

        // and get the return value
        String[] ret = new String[0];
        if (manual) {
            // wait till player finishes
            while (!manualReady)
            {   try { Thread.sleep(50);}
                catch (Exception e) { e.printStackTrace(); } 
            }
            // don't convert manual rolls to return value, as they are already simulated
        }
        else if (proc != null) {
            int N = Integer.parseInt(br.readLine());
            ret = new String[N];
            for (int i = 0; i < N; i++)
                ret[i] = br.readLine();
        }
        return ret;
    }
    // -----------------------------------------
    void draw() {
        if (!vis) return;
        v.processPause();
        v.repaint();
        try { Thread.sleep(del); }
        catch (Exception e) { };
    }
    // -----------------------------------------
    BufferedImage cache;
    void DrawTargetMaze() {
        cache = new BufferedImage((W+3)*SZ+100,H*SZ+40,BufferedImage.TYPE_INT_RGB);
        Graphics2D g2 = (Graphics2D)cache.getGraphics();
        // background
        g2.setColor(new Color(0xDDDDDD));
        g2.fillRect(0,0,(W+3)*SZ+100,H*SZ+40);
        g2.setColor(Color.WHITE);
        g2.fillRect(0,0,W*SZ,H*SZ);

        // target ball positions (square frames of required color)
        for (int i = 0; i < H; ++i)
        for (int j = 0; j < W; ++j)
            if (targetMaze[i][j] == '#') {
                g2.setColor(Color.BLACK);
                g2.fillRect(j * SZ, i * SZ, SZ, SZ);
            } else {
                if (targetMaze[i][j] != '.') {
                    g2.setColor(new Color(colors[(int)(targetMaze[i][j] - '0')]));
                    g2.fillRect(j * SZ + 1, i * SZ + 1, SZ - 1, SZ - 1);
                    g2.setColor(Color.WHITE);
                    g2.fillRect(j * SZ + 3, i * SZ + 3, SZ - 5, SZ - 5);
                }
            }

        // lines between maze cells
        g2.setColor(Color.BLACK);
        for (int i = 0; i <= H; i++)
            g2.drawLine(0,i*SZ,W*SZ,i*SZ);
        for (int i = 0; i <= W; i++)
            g2.drawLine(i*SZ,0,i*SZ,H*SZ);

        //"ready" "button"
        if (manual) {
            g2.drawString("READY",SZ*W+25,30);
            g2.drawRect(SZ*W+12,8,70,30);
        }
    }
    static BufferedImage deepCopy(BufferedImage source) {
        BufferedImage b = new BufferedImage(source.getWidth(), source.getHeight(), source.getType());
        Graphics g = b.getGraphics();
        g.drawImage(source, 0, 0, null);
        g.dispose();
        return b;
    }
    // -----------------------------------------
    public class Vis extends JPanel implements MouseListener, WindowListener {
        final Object keyMutex = new Object();
        boolean keyPressed;
        public boolean pauseMode = false;
        // -------------------------------------
        class DrawerKeyListener extends KeyAdapter {
            public void keyPressed(KeyEvent e) {
                synchronized (keyMutex) {
                    if (e.getKeyChar() == ' ') {
                        pauseMode = !pauseMode;
                    }
                    keyPressed = true;
                    keyMutex.notifyAll();
                }
            }
        }
        // -------------------------------------
        public void paint(Graphics g) {
            if (cache == null) {
                DrawTargetMaze();
            }
            BufferedImage bi = deepCopy(cache);
            Graphics2D g2 = (Graphics2D)bi.getGraphics();

            for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
                if (curMaze[i][j] != '.' && curMaze[i][j] != '#') {
                    g2.setColor(new Color(colors[(int)(curMaze[i][j] - '0')]));
                    g2.fillOval(j * SZ + 3, i * SZ + 3, SZ - 6, SZ - 6);
                }

            if (!firstClick) {
                // highlight ball which is going to roll after second click
                g2.setColor(Color.RED);
                g2.drawLine(first.c * SZ, first.r * SZ, (first.c + 1) * SZ, first.r * SZ);
                g2.drawLine(first.c * SZ, first.r * SZ, first.c * SZ, (first.r + 1) * SZ);
                g2.drawLine((first.c + 1) * SZ, (first.r + 1) * SZ, (first.c + 1) * SZ, first.r * SZ);
                g2.drawLine((first.c + 1) * SZ, (first.r + 1) * SZ, first.c * SZ, (first.r + 1) * SZ);
            }

            // draw line for previous roll
            if (prevX1>=0)
            {
                g2.setColor(new Color(colors[(int)(curMaze[prevY2][prevX2] - '0')]));
                g2.drawLine(prevX1*SZ+SZ/2, prevY1*SZ+SZ/2, prevX2*SZ+SZ/2, prevY2*SZ+SZ/2);
            }

            // number of rolls left
            g2.setColor(Color.BLACK);
            g2.setFont(new Font("Arial",Font.BOLD,14));
            FontMetrics fm = g2.getFontMetrics();
            g2.drawString(("Rolls"),(W+1)*SZ,3*SZ+fm.getHeight());
            g2.drawString((manualRolls+" : "+maxRolls),(W+1)*SZ,5*SZ+fm.getHeight());

            // current score
            g2.drawString(("Score"),(W+1)*SZ,7*SZ+fm.getHeight());
            g2.drawString(String.format("%.6f",getScore()),(W+1)*SZ,9*SZ+fm.getHeight());

            g.drawImage(bi,0,0,(W+3)*SZ+100,H*SZ+40,null);
        }
        // -------------------------------------
        public void processPause() {
            synchronized (keyMutex) {
                if (!pauseMode) {
                    return;
                }
                keyPressed = false;
                while (!keyPressed) {
                    try {
                        keyMutex.wait();
                    } catch (InterruptedException e) {
                        // do nothing
                    }
                }
            }
        }
        // -------------------------------------
        public Vis() {
            addMouseListener(this);
            jf.addKeyListener(new DrawerKeyListener());
            jf.addWindowListener(this);
        }
        // -------------------------------------
        //WindowListener
        public void windowClosing(WindowEvent e){ 
            if(proc != null)
                try { proc.destroy(); } 
                catch (Exception ex) { ex.printStackTrace(); }
            System.exit(0); 
        }
        public void windowActivated(WindowEvent e) { }
        public void windowDeactivated(WindowEvent e) { }
        public void windowOpened(WindowEvent e) { }
        public void windowClosed(WindowEvent e) { }
        public void windowIconified(WindowEvent e) { }
        public void windowDeiconified(WindowEvent e) { }
        // -------------------------------------
        //MouseListener
        public void mouseClicked(MouseEvent e) {
            // for manual play
            if (!manual || manualReady) return;

            // special button submits current state of the board
            int x = e.getX()-SZ*W-12, y = e.getY()-8;
            if (x>=0 && x<=80 && y>=0 && y<=30) {
                manualReady = true;
                return;
            }

            // regular click either stores coordinates of the ball (first click), or creates a roll (second click)

            // convert to args only clicks with valid coordinates
            // even for second click outside doesn't make sense
            int row = e.getY()/SZ, col = e.getX()/SZ;
            P p = new P(row, col);
            if (!isInside(p))
                return;

            // for first click, ignore clicks on non-balls
            if (firstClick) {
                if (curMaze[p.r][p.c] == '.' || curMaze[p.r][p.c] == '#')
                    return;
                // clicked on a ball => remember and switch to waiting second click
                first = p;
                firstClick = false;
                repaint();
                return;
            }
            // for second click, must be adjacent to first
            // or equal to first, to cancel without rolling
            if (p.r == first.r && p.c == first.c) {
                // cancel selection
                firstClick = true;
                repaint();
                return;
            }
            for (int dir = 0; dir < 4; ++dir)
                if (p.r == first.r + dr[dir] && p.c == first.c + dc[dir]) {
                    // roll in this direction
                    roll(first, dir);
                    firstClick = true;
                    ++manualRolls;
                    if (manualRolls == maxRolls)
                        manualReady = true;
                    repaint();
                    return;
                }
        }
        public void mousePressed(MouseEvent e) { }
        public void mouseReleased(MouseEvent e) { }
        public void mouseEntered(MouseEvent e) { }
        public void mouseExited(MouseEvent e) { }
    }
    // -----------------------------------------
    public RollingBallsVis(String seed) {
      try {
        if (vis)
        {   jf = new JFrame();
            v = new Vis();
            jf.getContentPane().add(v);
        }
        if (exec != null) {
            try {
                Runtime rt = Runtime.getRuntime();
                proc = rt.exec(exec);
                os = proc.getOutputStream();
                is = proc.getInputStream();
                br = new BufferedReader(new InputStreamReader(is));
                new ErrorReader(proc.getErrorStream()).start();
            } catch (Exception e) { e.printStackTrace(); }
        }
        System.out.println("Score = " + runTest(seed));
        if (proc != null)
            try { proc.destroy(); } 
            catch (Exception e) { e.printStackTrace(); }
      }
      catch (Exception e) { e.printStackTrace(); }
    }
    // -----------------------------------------
    public static void main(String[] args) {
        String seed = "1";
        vis = true;
        manual = false;
        del = 100;
        SZ = 13;
        for (int i = 0; i<args.length; i++)
        {   if (args[i].equals("-seed"))
                seed = args[++i];
            if (args[i].equals("-exec"))
                exec = args[++i];
            if (args[i].equals("-delay"))
                del = Integer.parseInt(args[++i]);
            if (args[i].equals("-novis"))
                vis = false;
            if (args[i].equals("-manual"))
                manual = true;
            if (args[i].equals("-size"))
                SZ = Integer.parseInt(args[++i]);
            if (args[i].equals("-debug"))
                debug = true;
            if (args[i].equals("-pause"))
                startPaused = true;
        }
        if (seed.equals("1"))
            SZ = 20;
        if (exec == null)
            manual = true;
        if (manual)
            vis = true;
        RollingBallsVis f = new RollingBallsVis(seed);
    }
    // -----------------------------------------
    void addFatalError(String message) {
        System.out.println(message);
    }
}

class ErrorReader extends Thread{
    InputStream error;
    public ErrorReader(InputStream is) {
        error = is;
    }
    public void run() {
        try {
            byte[] ch = new byte[50000];
            int read;
            while ((read = error.read(ch)) > 0)
            {   String s = new String(ch,0,read);
                System.out.print(s);
                System.out.flush();
            }
        } catch(Exception e) { }
    }
}
