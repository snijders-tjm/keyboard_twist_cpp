#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/Joy.h>
#include <std_msgs/Bool.h>

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <vector>

#include <map>

// Map for movement keys
std::map<char, std::vector<float>> moveBindings
{
  {'i', {1, 0, 0, 0}},
  {'o', {1, 0, 0, -1}},
  {'j', {0, 0, 0, 1}},
  {'l', {0, 0, 0, -1}},
  {'u', {1, 0, 0, 1}},
  {',', {-1, 0, 0, 0}},
  {'.', {-1, 0, 0, 1}},
  {'m', {-1, 0, 0, -1}},
  {'O', {1, -1, 0, 0}},
  {'I', {1, 0, 0, 0}},
  {'J', {0, 1, 0, 0}},
  {'L', {0, -1, 0, 0}},
  {'U', {1, 1, 0, 0}},
  {'<', {-1, 0, 0, 0}},
  {'>', {-1, -1, 0, 0}},
  {'M', {-1, 1, 0, 0}},
  {'t', {0, 0, 1, 0}},
  {'b', {0, 0, -1, 0}},
  {'k', {0, 0, 0, 0}},
  {'K', {0, 0, 0, 0}}
};

// Map for speed keys
std::map<char, std::vector<float>> speedBindings
{
  {'q', {1.1, 1.1}},
  {'z', {0.9, 0.9}},
  {'w', {1.1, 1}},
  {'x', {0.9, 1}},
  {'e', {1, 1.1}},
  {'c', {1, 0.9}}
};

// Map for controller keys
std::vector<char> controlBindings
{
  {'a'},
  {'s'},
  {'d'},
  {'f'}
};

// Reminder message
const char* msg = R"(

Reading from the keyboard and Publishing to Twist!
---------------------------
Moving around:
   u    i    o
   j    k    l
   m    ,    .

For Holonomic mode (strafing), hold down the shift key:
---------------------------
   U    I    O
   J    K    L
   M    <    >

t : up (+z)
b : down (-z)

anything else : stop

q/z : increase/decrease max speeds by 10%
w/x : increase/decrease only linear speed by 10%
e/c : increase/decrease only angular speed by 10%

Rest = a
Trot = s
Crawl = d
Stand = f

CTRL-C to quit




)";

// Init variables
float speed(0.5); // Linear velocity (m/s)
float turn(1.0); // Angular velocity (rad/s)
float x(0), y(0), z(0), th(0); // Forward/backward/neutral direction vars
char key(' ');
float button0, button1, button2, button3; // For switching controllers

// For non-blocking keyboard inputs
int getch(void)
{
  int ch;
  struct termios oldt;
  struct termios newt;

  // Store old settings, and copy to new settings
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  // Make required changes and apply the settings
  newt.c_lflag &= ~(ICANON | ECHO);
  newt.c_iflag |= IGNBRK;
  newt.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF);
  newt.c_lflag &= ~(ICANON | ECHO | ECHOK | ECHOE | ECHONL | ISIG | IEXTEN);
  newt.c_cc[VMIN] = 1;
  newt.c_cc[VTIME] = 0;
  tcsetattr(fileno(stdin), TCSANOW, &newt);

  // Get the current character
  ch = getchar();

  // Reapply old settings
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return ch;
}

int main(int argc, char** argv)
{
  // Init ROS node
  ros::init(argc, argv, "teleop_twist_keyboard_TOMMY");
  ros::NodeHandle nh;

  // Init cmd_vel publisher
  ros::Publisher pub = nh.advertise<geometry_msgs::Twist>("cmd_vel_key", 1);
  ros::Publisher pub2 = nh.advertise<std_msgs::Bool>("rest_cmd", 1);
  ros::Publisher pub3 = nh.advertise<std_msgs::Bool>("trot_cmd", 1);
  ros::Publisher pub4 = nh.advertise<std_msgs::Bool>("crawl_cmd", 1);
  ros::Publisher pub5 = nh.advertise<std_msgs::Bool>("stand_cmd", 1);

  // Create Twist message
  geometry_msgs::Twist twist;
  std_msgs::Bool rest_event_bool;
  std_msgs::Bool trot_event_bool;
  std_msgs::Bool crawl_event_bool;
  std_msgs::Bool stand_event_bool;

  printf("%s", msg);
  printf("\rCurrent: speed %f\tturn %f | Awaiting command...\r", speed, turn);

  while(true){

    // Get the pressed key
    key = getch();
    std::vector<char>::iterator it = std::find(controlBindings.begin(), controlBindings.end(),key);

    // If the key corresponds to a key in moveBindings
    if (moveBindings.count(key) == 1)
    {
      // Grab the direction data
      x = moveBindings[key][0];
      y = moveBindings[key][1];
      z = moveBindings[key][2];
      th = moveBindings[key][3];

      printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
    }

    // Otherwise if it corresponds to a key in speedBindings
    else if (speedBindings.count(key) == 1)
    {
      // Grab the speed data
      speed = speed * speedBindings[key][0];
      turn = turn * speedBindings[key][1];

      printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
    }

    // Otherwise, if it corresponds to a key in controlBindings
    else if (it != controlBindings.end())
    {
      x = 0;
      y = 0;
      z = 0;
      th = 0;

      if (controlBindings[0]==key)
      {
         rest_event_bool.data = true;
         trot_event_bool.data = false;
         crawl_event_bool.data = false;
         stand_event_bool.data = false;

         printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
      }
      else if (controlBindings[1]==key)
      {
         rest_event_bool.data = false;
         trot_event_bool.data = true;
         crawl_event_bool.data = false;
         stand_event_bool.data = false;

         printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
      }
      else if (controlBindings[2]==key)
      {
         rest_event_bool.data = false;
         trot_event_bool.data = false;
         crawl_event_bool.data = true;
         stand_event_bool.data = false;

         printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
      }
      else
      {
         rest_event_bool.data = false;
         trot_event_bool.data = false;
         crawl_event_bool.data = false;
         stand_event_bool.data = true;
 
         printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
      }
    }

    // Otherwise, set the robot to stop
    else
    {
      x = 0;
      y = 0;
      z = 0;
      th = 0;

      // If ctrl-C (^C) was pressed, terminate the program
      if (key == '\x03')
      {
        printf("\n\n                 .     .\n              .  |\\-^-/|  .    \n             /| } O.=.O { |\\\n\n                 CH3EERS\n\n");
        break;
      }

      printf("\rCurrent: speed %f\tturn %f | Invalid command! %c", speed, turn, key);
    }

    // Update the Twist message
    twist.linear.x = x * speed;
    twist.linear.y = y * speed;
    twist.linear.z = z * speed;

    twist.angular.x = 0;
    twist.angular.y = 0;
    twist.angular.z = th * turn;

    //joy.buttons[0] = button0;
    //joy.buttons[1] = button1;
    //joy.buttons[2] = button2;
    //joy.buttons[3] = button3;

    // Publish it and resolve any remaining callbacks
    //pub2.publish(joy);
    pub.publish(twist);
    pub2.publish(rest_event_bool);
    pub3.publish(trot_event_bool);
    pub4.publish(crawl_event_bool);
    pub5.publish(stand_event_bool);


    ros::spinOnce();
  }

  return 0;
}
