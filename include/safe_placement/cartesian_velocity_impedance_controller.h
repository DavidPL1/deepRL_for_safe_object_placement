#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <realtime_tools/realtime_buffer.h>
#include <realtime_tools/realtime_publisher.h>

#include <controller_interface/multi_interface_controller.h>
#include <dynamic_reconfigure/server.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Twist.h>

#include <hardware_interface/joint_command_interface.h>
#include <hardware_interface/robot_hw.h>
#include <ros/node_handle.h>
#include <ros/time.h>
#include <eigen3/Eigen/Dense>

#include <franka_example_controllers/compliance_paramConfig.h>
#include <franka_hw/franka_model_interface.h>
#include <franka_hw/franka_state_interface.h>

namespace safe_placement {

class CartesianVelocityImpedanceController : public controller_interface::MultiInterfaceController<
                                                    franka_hw::FrankaModelInterface,
                                                    hardware_interface::EffortJointInterface,
                                                    franka_hw::FrankaStateInterface> {
    public:
        bool init(hardware_interface::RobotHW* robot_hw, ros::NodeHandle& node_handle) override;
        void starting(const ros::Time&) override;
        void update(const ros::Time&, const ros::Duration& period) override;

    private:
        // Saturation
        Eigen::Matrix<double, 7, 1> saturateTorqueRate(
            const Eigen::Matrix<double, 7, 1>& tau_d_calculated,
            const Eigen::Matrix<double, 7, 1>& tau_J_d);  // NOLINT (readability-identifier-naming)

        std::unique_ptr<franka_hw::FrankaStateHandle> state_handle_;
        std::unique_ptr<franka_hw::FrankaModelHandle> model_handle_;
        std::vector<hardware_interface::JointHandle> joint_handles_;

        double filter_params_{0.005};
        double nullspace_stiffness_{20.0};
        double nullspace_stiffness_target_{20.0};
        const double delta_tau_max_{1.0};
        Eigen::Matrix<double, 6, 6> cartesian_stiffness_;
        Eigen::Matrix<double, 6, 6> cartesian_stiffness_target_;
        Eigen::Matrix<double, 6, 6> cartesian_damping_;
        Eigen::Matrix<double, 6, 6> cartesian_damping_target_;
        Eigen::Matrix<double, 7, 1> q_d_nullspace_;
        Eigen::Vector3d position_d_;
        Eigen::Quaterniond orientation_d_;
    
        Eigen::Matrix<double, 6, 1> error;
        // Dynamic reconfigure
        std::unique_ptr<dynamic_reconfigure::Server<franka_example_controllers::compliance_paramConfig>>
            dynamic_server_compliance_param_;
        ros::NodeHandle dynamic_reconfigure_compliance_param_node_;

        struct Commands
        {
            double twist[6];
            ros::Time stamp;

            Commands() : twist{}, stamp(0.0) {}
        };

        realtime_tools::RealtimeBuffer<Commands> command_;
        Commands command_struct_;

        ros::Subscriber sub_command_;

        void cmdCallback(const geometry_msgs::Twist& command);
        
        void complianceParamCallback(franka_example_controllers::compliance_paramConfig& config,
                                    uint32_t level);


};

}  // namespace franka_example_controllers