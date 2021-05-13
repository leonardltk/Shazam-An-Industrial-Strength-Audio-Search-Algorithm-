#!/bin/sh
content="<font color=\\\"info\\\">Gitlab CI 触发通知</font>"
content="${content}\n> Deployment: ${DEPLOYMENT_NAME}"
content="${content}\n> User: ${GITLAB_USER_NAME}"
content="${content}\n> Cluster: ${CI_ENVIRONMENT_NAME}"
content="${content}\n> Project: [${CI_PROJECT_NAME}](${CI_PROJECT_URL})"
content="${content}\n> Branch: ${CI_BUILD_REF_NAME}"
content="${content}\n> Pipeline: [${CI_PIPELINE_ID}](${CI_PIPELINE_URL})"
content="${content}\n> Build: ${CI_BUILD_NAME}"
content="${content}\n> Commit: $(echo ${CI_COMMIT_MESSAGE} | sed ':a;N;$!ba;s/\n/, /g' | sed 's/"/\\\"/g')"

echo $content
wget https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=${WECHAT_ROBOT_KEY} \
    --header="Content-Type: application/json" \
    --post-data="{\"msgtype\":\"markdown\",\"markdown\":{\"content\":\"${content}\"}}"
