#ifndef __APP_SLEEP_H__
#define __APP_SLEEP_H__

#include <stdint.h>
#include "aon_rtc.h"

void app_enter_deep_sleep_with_wakeup_by_rtc(uint32_t sleep_ms);
void app_enter_sleep_with_wakeup_by_timer(uint32_t sleep_ms);






// 15. 三数之和
// 给你一个整数数组 nums ，判断是否存在三元组 [nums[i], nums[j], nums[k]] 
//满足 i != j、i != k 且 j != k ，同时还满足 nums[i] + nums[j] + nums[k] == 0 。返回所有和为 0 且不重复的三元组
// 注意：答案中不可以包含重复的三元组。

// 输入：nums = [-1,0,1,2,-1,-4]
// 输出：[[-1,-1,2],[-1,0,1]]
// 解释：
// nums[0] + nums[1] + nums[2] = (-1) + 0 + 1 = 0 。
// nums[1] + nums[2] + nums[4] = 0 + 1 + (-1) = 0 。
// nums[0] + nums[3] + nums[4] = (-1) + 2 + (-1) = 0 。
// 不同的三元组是 [-1,0,1] 和 [-1,-1,2] 。
// 注意，输出的顺序和三元组的顺序并不重要。
// 示例 2：

// 输入：nums = [0,1,1]
// 输出：[]
// 解释：唯一可能的三元组和不为 0 。

//先排序:nums = [-4,-1,-1,0,1,2]
// class Solution {
// public:
//     vector<vector<int>> threeSum(vector<int>& nums) {
//         sort(nums.begin(), nums.end());
//         vector<vector<int>> result;
       
//         for (int i = 0; i < nums.size(); i++) {

//             //当前元素与前一个元素相同，跳过，去重逻辑
//             if(nums[i] == nums[i-1]&& i > 0) {
//                 continue;
//             }
//              //排序之后的数一定是左边是负数，右边是正数，如果遍历到正数了，就不用往后面找了
//             if(nums[i] > 0) {
//                 break;
//             }

//             //在后面的nums.size()-i-1个元素中寻找两个数，使得它们的和等于-nums[i]
//             int left = i + 1;
//             int right = nums.size() - 1;

//             // 边界处理：确保剩余元素足够组成三元组
//             if (left >= right) break;

//             // 如果当前最小的两个数加起来都太大
//             if (nums[left] + nums[left + 1] > -nums[i]) continue; 
//             // 如果当前最大的两个数加起来都太小
//             if (nums[right - 1] + nums[right] < -nums[i]) continue;

//             while(left < right) {
//                 int sum = nums[left] + nums[right];
                
//                 //和比目标大
//                 if(sum > -nums[i]) {
//                     right--;
//                 } else if(sum < -nums[i]) { //和比目标小
//                     left++;
//                 } else {  //和等于目标，则继续找下一组，注意去重判断
//                     result.push_back({nums[i], nums[left], nums[right]}); //找到一个！
                    
//                     while(left < right && nums[left] == nums[left + 1]) {
//                         left++;
//                     }
//                     while(left < right && nums[right] == nums[right - 1]) {
//                         right--;
//                     }
//                     left++;
//                     right--;
                   
//                 }
//             }

//         }

//         return result;
//     }
// };

#endif // SLEEP_H